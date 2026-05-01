#pragma once

#include "core/IPollable.hpp"		// IPollable
#include "core/Socket.hpp"			// Socket
#include "utils/NetworkUtils.hpp"	// Interface
#include "http/HttpRequest.hpp"		// HttpRequest
#include "http/HttpResponse.hpp"	// HttpResponse
#include "cgi/Cgi.hpp"				// CGIProcess



















class IOReactor;

class Client : public IPollable
{

public:

	Client(int fd, const Interface &iface, IOReactor &reactor);
	~Client();

	int	readFd() const;
	int	writeFd() const;

	void onRead();
	void onWrite();

	Socket		socket;
	Interface	iface;

	HttpRequest		request;
	HttpResponse	response;

	std::string	writeBuffer; // move to response later
	size_t		writeOffset; // move to response later
	
	time_t	lastActive;
	bool	keepAlive;

	CGIProcess	*cgi; // allocated in CGIHandler::start(client), deleted in CGIHandler::finish/kill and ~Client

private:

	Client();
	Client(const Client &);
	Client &operator=(const Client &);

};
