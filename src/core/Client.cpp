#include "core/Client.hpp"
#include "Defaults.hpp"
#include "core/IOReactor.hpp"
#include "cgi/Cgi.hpp"
#include "http/HttpPipeline.hpp"
#include "http/HttpStatusCodes.hpp"
#include "utils/StringUtils.hpp"

#include <ctime>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>
#include <sys/stat.h>

#define WAIT_FOR_CLIENT_DATA		POLLIN
#define WAIT_TO_SEND_RESPONSE		POLLOUT
#define WAIT_FOR_CLIENT_AND_SEND	(POLLIN | POLLOUT)
#define WAIT_FOR_CGI				0
#define CGI_IO						(POLLIN | POLLOUT)

Client::Client(int fd, const Interface &listeningIface, const Interface &clientIface, IOReactor &reactor, const ListenEndpoints &endpts) : IPollable(reactor), listeningIface(listeningIface), socket(fd), clientIface(clientIface), endpoints(endpts), writeOffset(0), keepAlive(false), draining_(false), cgi(NULL)
{
	int rcvbuf = CLIENT_RCVBUF_SIZE;
	setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));
}

void Client::onTimeout()
{
	delete this;
}

// Client
void Client::onShutdown()
{
	if (cgi)
	{
		CGIHandler::killProcess(*this, HttpStatus::ServiceUnavailable);
		clearCgi();
		keepAlive = false;
		beginResponse();
		return;
	}
	delete this;
}

Client::~Client()
{
	if (cgi)
	{
		CGIHandler::killProcess(*this, HttpStatus::ServiceUnavailable);
		clearCgi();
	}
	reactor_.remove(*this);
}

int Client::readFd()	const { return socket.get(); }
int Client::writeFd()	const { return socket.get(); }


void Client::beginResponse()
{
	response.setHeader("Connection", keepAlive ? "keep-alive" : "close");
	writeBuffer = response.serialize();
	writeOffset = 0;
	HttpPipeline::logRequest(request, response, listeningIface, writeBuffer.size());
	if (draining_ || !request.complete())
	{
		draining_ = true;
		reactor_.mod(*this, WAIT_FOR_CLIENT_AND_SEND);
	}
	else
		reactor_.mod(*this, WAIT_TO_SEND_RESPONSE);
}

void Client::onRead()
{
	char	buffer[CLIENT_READ_BUFFER_SIZE];
	ssize_t bytes = recv(socket.get(), buffer, sizeof(buffer), 0);

	if (bytes <= 0)
	{
		delete this;
		return ;
	}

	if (draining_)
		return ; // body being drained — discard bytes, wait for client to close or timeout

	try
	{
		updateActivity();
		request.parse(std::string(buffer, bytes));

		if (!request.vhost && request.headersComplete() && !request.erroneous())
		{
			HttpPipeline::resolve(request, endpoints, listeningIface);
			if (request.route)
			{
				request.setMaxBodySize(request.route->maxBodySize());

				if (request.contentLength() > request.route->maxBodySize())
					return sendErrorResponse(HttpStatus::ContentTooLarge);

				if (request.expectsContinue())
					send(socket.get(), "HTTP/1.1 100 Continue\r\n\r\n", 25, 0);

				if (request.hasCgi())
				{
					cgi = new CGIProcess(reactor_, *this);
					request.initBody(cgi->writeFd());				// fd_ set ← pipes exist, no fork yet
					request.parse("");						// body decoded, data_ filled
					if (int err = CGIHandler::start(*this))	// fork
						return (clearCgi(), sendErrorResponse(HttpStatus::Code(err)));
					reactor_.add(*cgi, CGI_IO);
				}
				else if (!request.route->upload().empty() && (request.method() == "POST" || request.method() == "PUT"))
					if (int err = HttpPipeline::prepareUploadRequest(request))
						return sendErrorResponse(HttpStatus::Code(err));
			}
			request.parse("");
			keepAlive = !request.connectionClose();
		}

		if (!request.complete())
			return ;

		if (cgi)
			return reactor_.mod(*this, WAIT_FOR_CGI);

		response = HttpPipeline::buildResponse(request);
		beginResponse();
	}
	catch (const std::exception &e)
	{
		std::cerr << StringUtils::currentTime() << " [error] " << e.what() << "\n";
		sendErrorResponse(HttpStatus::InternalServerError);
	}
}

void Client::onWrite()
{
	// send headers / small body first
	if (writeOffset < writeBuffer.size())
	{
		size_t	toSend	= std::min((size_t)CLIENT_SNDBUF_SIZE, writeBuffer.size() - writeOffset);
		ssize_t	sent = send(socket.get(), writeBuffer.c_str() + writeOffset, toSend, 0);
		if (sent <= 0)
			return delete this;
		writeOffset += sent;
		return ; // more chunks next POLLOUT
	}
	// stream file body (skipped for HEAD — headers already sent with correct Content-Length)
	if (response.hasFile() && request.method() != "HEAD")
	{
		if (!fileStream_.is_open())
		{
			fileStream_.clear(); // C++98/libstdc++: open() doesn't clear eofbit on linux but on mac it does 🙂
			fileStream_.open(response.filePath().c_str(), std::ios::binary);
			if (!fileStream_.good())
				return delete this;
		}
		char chunk[CLIENT_SNDBUF_SIZE];
		fileStream_.read(chunk, sizeof(chunk));
		std::streamsize n = fileStream_.gcount();
		if (n > 0)
		{
			ssize_t sent = send(socket.get(), chunk, n, 0);
			if (sent <= 0)
				return delete this;
			return ; // more chunks next POLLOUT
		}
		fileStream_.close();
	}
	// done
	if (keepAlive)
	{
		request.reset();
		response.reset();
		writeBuffer.clear();
		writeOffset = 0;
		draining_ = false;
		reactor_.mod(*this, WAIT_FOR_CLIENT_DATA);
	}
	else if (draining_)// response sent but body still incoming (non async keep-alive client)
		reactor_.mod(*this, WAIT_FOR_CLIENT_DATA);
	else
		delete this;
}

void Client::sendErrorResponse(HttpStatus::Code code)
{
	keepAlive = false;
	response  = HttpPipeline::errorResponse(request, code);
	beginResponse();
}

void Client::onCgiComplete()
{
	clearCgi();
	updateActivity();
	beginResponse();
}

void Client::clearCgi()
{
	if (!cgi) return;
	reactor_.remove(*cgi);
	delete cgi;
	cgi = NULL;
}