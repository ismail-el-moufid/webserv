#pragma once

#include "core/IPollable.hpp"		// IPollable
#include "utils/NetworkUtils.hpp"	// Interface
#include "http/HttpRequest.hpp"		// HttpRequest
#include "config/Config.hpp"		// ListenEndpoints
#include "core/Socket.hpp"			// Socket
#include "http/HttpResponse.hpp"	// HttpResponse
#include "cgi/Cgi.hpp"				// CGIProcess

#include <fstream>					// ifstream

class IOReactor;

class Client : public IPollable
{

public:

	Client(int fd, const Interface &listeningIface, const Interface &clientIface, IOReactor &reactor, const ListenEndpoints &endpoints);
	~Client();

	int	readFd() const;
	int	writeFd() const;

	void onRead();
	void onWrite();
	void onTimeout();
	void onCgiComplete();
	void onShutdown();


	void sendErrorResponse(HttpStatus::Code code);
	
	Interface	listeningIface;

	Socket		socket;
	Interface	clientIface;

	HttpRequest		request;
	HttpResponse	response;

	const ListenEndpoints &endpoints;

	std::string	writeBuffer;
	size_t		writeOffset;

	bool	keepAlive;
	bool	draining_; // true when sending error response while body still incoming

	CGIProcess *cgi; // allocated in Client::onRead(), deleted in CGIHandler::finish/kill and ~Client

private:

	Client();
	Client(const Client &);
	Client &operator=(const Client &);

	void clearCgi();
	void beginResponse();

	std::ifstream fileStream_;

};
