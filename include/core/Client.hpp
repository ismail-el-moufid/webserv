#pragma once

#include "core/IPollable.hpp"		// IPollable
#include "utils/NetworkUtils.hpp"	// Interface
#include "http/HttpRequest.hpp"		// HttpRequest
#include "config/Config.hpp"		// ListenEndpoints
#include "core/Socket.hpp"			// Socket
#include "http/HttpResponse.hpp"	// HttpResponse
#include "cgi/cgi.hpp"				// CGIProcess

#include <fstream>					// ifstream

class IOReactor;

class Client : public IPollable
{

public:

	Client(int fd, const Interface &iface, IOReactor &reactor, const ListenEndpoints &endpoints);
	~Client();

	int	readFd() const;
	int	writeFd() const;

	void onRead();
	void onWrite();
	void onTimeout();
	void onCgiComplete();

	void clearCgi();

	Socket		socket;
	Interface	iface;

	HttpRequest		request;
	HttpResponse	response;

	const ListenEndpoints &endpoints;

	std::string	writeBuffer;
	size_t		writeOffset;

	bool	keepAlive;

	CGIProcess *cgi; // allocated in Client::onRead(), deleted in CGIHandler::finish/kill and ~Client

private:

	Client();
	Client(const Client &);
	Client &operator=(const Client &);

	std::ifstream fileStream_;

};
