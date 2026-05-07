#include "core/Client.hpp"
#include "core/IOReactor.hpp"
#include "cgi/Cgi.hpp"
#include "http/HttpPipeline.hpp"
#include "utils/StringUtils.hpp"

#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>

#define WAIT_FOR_CLIENT_DATA		POLLIN
#define WAIT_TO_SEND_RESPONSE		POLLOUT
#define WAIT_FOR_CLIENT_AND_SEND	(POLLIN | POLLOUT)
#define WAIT_FOR_CGI				0
#define CGI_IO						(POLLIN | POLLOUT)

Client::Client(int fd, const Interface &iface, IOReactor &reactor, const ListenEndpoints &endpts) : IPollable(reactor), socket(fd), iface(iface), endpoints(endpts), writeOffset(0), keepAlive(false), cgi(NULL) { }

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
}

int Client::readFd()	const { return socket.get(); }
int Client::writeFd()	const { return socket.get(); }

void Client::onRead()
{
	char	buffer[4096];
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
				request.setMaxBodySize(request.route->maxBodySize());
			request.parse(""); // request pauses its parsing right after finishing header parsing to allow us to set max body size, here we just resume it
			keepAlive = !request.connectionClose();
		}

		if (!request.complete())
			return ;

		if (request.route && !request.route->allowedMethods().empty()
			&& std::find(request.route->allowedMethods().begin(), request.route->allowedMethods().end(), request.method()) == request.route->allowedMethods().end())
		{
			response = HttpPipeline::errorResponse(request, HttpStatus::MethodNotAllowed);
			writeBuffer = response.serialize();
			HttpPipeline::logRequest(request, response, iface, writeBuffer.size());
			reactor_.mod(*this, WAIT_TO_SEND_RESPONSE);
			return ;
		}

		if (request.hasCgi())
		{
			cgi = new CGIProcess(reactor_, *this);
			CGIHandler::start(*this);
			if (!cgi)
			{
				response = HttpResponse::HttpErrorResponse(HttpStatus::InternalServerError);
				writeBuffer = response.serialize();
				HttpPipeline::logRequest(request, response, iface, writeBuffer.size());
				reactor_.mod(*this, WAIT_TO_SEND_RESPONSE);
				return ;
			}
			reactor_.add(*cgi, CGI_IO);
			reactor_.mod(*this, WAIT_FOR_CGI);
		}
		else
		{
			response = HttpPipeline::buildResponse(request);
			writeBuffer = response.serialize();
			HttpPipeline::logRequest(request, response, iface, writeBuffer.size());
			reactor_.mod(*this, WAIT_TO_SEND_RESPONSE);
		}
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
	if (writeBuffer.empty() || writeOffset >= writeBuffer.size())
		return ;
	ssize_t sent = send(socket.get(), writeBuffer.c_str() + writeOffset, writeBuffer.size() - writeOffset, 0);
	if (sent <= 0)
		return delete this;

	writeOffset += sent;
	if (writeOffset < writeBuffer.size())
		return ;
	if (keepAlive)
	{
		request.reset();
		request.parse(""); // re-parse leftover from rawBuffer_ from position 0
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
