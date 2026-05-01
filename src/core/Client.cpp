#include "core/Client.hpp"
#include "core/IOReactor.hpp"
#include "cgi/cgi.hpp"
#include <sys/socket.h>
#include <unistd.h>

#define DEFAULT_MAX_BODY_SIZE (1024 * 1024)  // 1MB

Client::Client(int fd, const Interface &iface, IOReactor &reactor) : IPollable(reactor), socket(fd), iface(iface), request(DEFAULT_MAX_BODY_SIZE), writeOffset(0), keepAlive(false), cgi(NULL) { }

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

int Client::readFd()  const { return socket.get(); }
int Client::writeFd() const { return socket.get(); }

void Client::onRead()
{
	char	buffer[4096];
	ssize_t	bytes = recv(socket.get(), buffer, sizeof(buffer), 0);

	if (bytes <= 0)
	{
		delete this;
		return ;
	}
	request.parse(std::string(buffer, bytes));
	if (!request.complete())
		return ;
	if (request.errorCode())
	{
	reactor_.mod(*this, POLLIN | POLLOUT);
	return ;
	}
	// hardcoded as if we detected a CGI for now, but later we call the router here to determine if we call static/upload/cgi handler
	cgi = new CGIProcess(reactor_, *this);
	CGIHandler::start(*this);
	int events = POLLIN;
	if (request.method() == "POST")
		events |= POLLOUT;
	reactor_.add(*cgi, events);
	reactor_.mod(*this, 0);
}

void Client::onWrite()
{
	if (writeBuffer.empty())
		return ;
	size_t sent = send(socket.get(), writeBuffer.c_str() + writeOffset, writeBuffer.size() - writeOffset, 0);
	if (sent <= 0)
	{
		delete this;
		return ;
	}
	writeOffset += sent;
	if (writeOffset < writeBuffer.size())
		return ;
	if (keepAlive)
	{
		request.reset();
		response = HttpResponse();
		writeBuffer.clear();
		writeOffset = 0;
		reactor_.mod(*this, POLLIN);
	}
	else
		delete this;
}