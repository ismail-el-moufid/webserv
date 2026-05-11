#include "core/Client.hpp"
#include "Defaults.hpp"
#include "core/IOReactor.hpp"
#include "cgi/cgi.hpp"
#include "http/HttpPipeline.hpp"
#include "utils/StringUtils.hpp"

#include <ctime>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>

#define WAIT_FOR_CLIENT_DATA		POLLIN
#define WAIT_TO_SEND_RESPONSE		POLLOUT
#define WAIT_FOR_CLIENT_AND_SEND	(POLLIN | POLLOUT)
#define WAIT_FOR_CGI				0
#define CGI_IO						(POLLIN | POLLOUT)

Client::Client(int fd, const Interface &iface, IOReactor &reactor, const ListenEndpoints &endpts) : IPollable(reactor), socket(fd), iface(iface), endpoints(endpts), writeOffset(0), keepAlive(false), cgi(NULL)
{
	std::cout << StringUtils::currentTime() << " [info] New connection: " << socket.get() << std::endl;
	int rcvbuf = CLIENT_RCVBUF_SIZE;
	setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));
}

Client::~Client()
{
	if (cgi)
	{
		CGIHandler::killProcess(*this);
		reactor_.remove(*cgi);
		delete cgi;
		cgi = NULL;
	}
	reactor_.remove(*this);
	std::cerr << StringUtils::currentTime() << " [info] Connection closed: " << socket.get() << "\n";
}

int Client::readFd()	const { return socket.get(); }
int Client::writeFd()	const { return socket.get(); }

void Client::onRead()
{
	char	buffer[CLIENT_READ_BUFFER_SIZE];
	ssize_t bytes = recv(socket.get(), buffer, sizeof(buffer), 0);

	if (bytes <= 0)
	{
		delete this;
		return ;
	}

	try
	{
		updateActivity();
		request.parse(std::string(buffer, bytes));

		if (!request.vhost && request.headersComplete() && !request.erroneous())
		{
			HttpPipeline::resolve(request, endpoints, iface);
			if (request.route)
			{
				request.setMaxBodySize(request.route->maxBodySize());

				if (request.hasCgi())
				{
					cgi = new CGIProcess(reactor_, *this);
					CGIHandler::start(*this);
					if (cgi)
					{
						reactor_.add(*cgi, CGI_IO);
						request.initBody(cgi->writeFd());
					}
				}
				else if (!request.route->upload().empty() && (request.method() == "POST" || request.method() == "PUT"))
				{
					const std::map<std::string, std::string> &hdrs = request.headers();
					std::map<std::string, std::string>::const_iterator ct = hdrs.find("content-type");
					if (ct != hdrs.end())
					{
						size_t bp = ct->second.find("boundary=");
						if (bp != std::string::npos)
						{
							std::string boundary = ct->second.substr(bp + 9);
							size_t semi = boundary.find(';');
							if (semi != std::string::npos) boundary = boundary.substr(0, semi);
							request.initBody(request.route->upload(), boundary);
						}
						else
							request.initBody(request.route->upload());
					}
					else
						request.initBody(request.route->upload());
				}
			}
			{
				const std::map<std::string, std::string> &hdrs = request.headers();
				std::map<std::string, std::string>::const_iterator expIt = hdrs.find("expect");
				if (expIt != hdrs.end() && expIt->second == "100-continue")
				{
					if (request.contentLength() > request.route->maxBodySize())
					{
						const std::string reject = "HTTP/1.1 417 Expectation Failed\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
						send(socket.get(), reject.c_str(), reject.size(), 0);
						delete this;
						return ;
					}
					const std::string cont = "HTTP/1.1 100 Continue\r\n\r\n";
					send(socket.get(), cont.c_str(), cont.size(), 0);
				}
			}
			request.parse(""); // resumes body parsing after max body size is set
			keepAlive = !request.connectionClose();
		}

		if (!request.complete())
			return ;

		if (request.route && !request.route->allowedMethods().empty()
			&& std::find(request.route->allowedMethods().begin(), request.route->allowedMethods().end(), request.method()) == request.route->allowedMethods().end())
		{
			response = HttpPipeline::errorResponse(request, HttpStatus::MethodNotAllowed);
			response.setHeader("Connection", keepAlive ? "keep-alive" : "close");
			writeBuffer = response.serialize();
			HttpPipeline::logRequest(request, response, iface, writeBuffer.size());
			reactor_.mod(*this, WAIT_TO_SEND_RESPONSE);
			return ;
		}

		if (cgi)
		{
			reactor_.mod(*this, WAIT_FOR_CGI);
			return ;
		}

		response = HttpPipeline::buildResponse(request);
		response.setHeader("Connection", keepAlive ? "keep-alive" : "close");
		writeBuffer = response.serialize();
		HttpPipeline::logRequest(request, response, iface, writeBuffer.size());
		reactor_.mod(*this, WAIT_TO_SEND_RESPONSE);
	}
	catch (const std::exception &e)
	{
		std::cerr << StringUtils::currentTime() << " [error] " << e.what() << "\n";
		response = HttpResponse::HttpErrorResponse(HttpStatus::InternalServerError);
		writeBuffer = response.serialize();
		HttpPipeline::logRequest(request, response, iface, writeBuffer.size());
		reactor_.mod(*this, WAIT_TO_SEND_RESPONSE);
	}
}

void Client::onWrite()
{
	// send headers / small body first
	if (writeOffset < writeBuffer.size())
	{
		ssize_t sent = send(socket.get(), writeBuffer.c_str() + writeOffset, writeBuffer.size() - writeOffset, 0);
		if (sent <= 0)
			return delete this;
		writeOffset += sent;
		if (writeOffset < writeBuffer.size())
			return;
	}

	// stream file body in 4KB chunks
	if (response.hasFile())
	{
		if (!fileStream_.is_open())
			fileStream_.open(response.filePath().c_str(), std::ios::binary);
		if (!fileStream_.good())
			return delete this;

		char chunk[4096];
		fileStream_.read(chunk, sizeof(chunk));
		std::streamsize n = fileStream_.gcount();
		if (n > 0)
		{
			ssize_t sent = send(socket.get(), chunk, n, 0);
			if (sent <= 0) return delete this;
			return; // more chunks next POLLOUT
		}
		fileStream_.close();
	}

	// done
	if (keepAlive)
	{
		request.reset();
		request.parse("");
		response.reset();
		writeBuffer.clear();
		writeOffset = 0;
		reactor_.mod(*this, WAIT_FOR_CLIENT_DATA);
	}
	else
		delete this;
}

void Client::onCgiComplete()
{
	updateActivity();
	writeBuffer = response.serialize();
	HttpPipeline::logRequest(request, response, iface, writeBuffer.size());
	reactor_.mod(*this, WAIT_FOR_CLIENT_AND_SEND);
}

void Client::clearCgi()
{
	delete cgi;
	cgi = NULL;
}

void Client::onTimeout() { delete this; }
