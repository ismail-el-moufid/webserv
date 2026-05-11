#pragma once

#include "config/Config.hpp"		// Config
#include "core/IOReactor.hpp"		// IOReactor

#include <ctime>					// time_t

class Server
{

public:

	Server();
	Server(const std::string &configPath);
	~Server();

	void run();

private:

	Server(Server &);
	Server &operator=(Server &);

	// keepAlive and cgi timeout
	time_t timeout_;

	// server configs
	ListenEndpoints	endpoints_;
	VirtualHosts	vhosts_;

	// factories
	IOReactor *reactor_; // pointer so Config can set timeout_ before construction

};
