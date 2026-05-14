#pragma once

#include "config/Config.hpp"		// Config
#include "core/IOReactor.hpp"		// IOReactor

#include <csignal>					// sig_atomic_t
#include <ctime>					// time_t

class Server
{

public:

	Server(volatile sig_atomic_t &running);
	Server(const std::string &configPath, volatile sig_atomic_t &running);
	~Server();

	void run();

private:

	Server(Server &);
	Server &operator=(Server &);

	volatile sig_atomic_t	&running_;
	time_t					timeout_;
	ListenEndpoints			endpoints_;
	VirtualHosts			vhosts_;
	IOReactor				*reactor_;

};