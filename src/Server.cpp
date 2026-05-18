#include "Server.hpp"					// Server, run
#include "core/ListeningSocket.hpp"		// ListeningSocket
#include "utils/StringUtils.hpp"		// currentTime
#include <iostream>						// cout

Server::Server(const std::string &configPath, volatile sig_atomic_t &running) : running_(running), inactiveTimeout_(30), reactor_(NULL)
{
	std::cout << StringUtils::currentTime() << " webserver starting\n";

	{
		Config config(configPath, vhosts_, endpoints_, inactiveTimeout_);
	}

	reactor_ = new IOReactor(inactiveTimeout_);

	for (ListenEndpoints::const_iterator it = endpoints_.begin(); it != endpoints_.end(); ++it)
	{
		try { new ListeningSocket(it->first, *reactor_, endpoints_); }
		catch (const std::exception &e) { std::cerr << e.what() << "\n"; }
	}

	if (reactor_->empty())
	{
		delete reactor_;
		throw std::runtime_error(StringUtils::currentTime() + " [error] No listening sockets could be opened");
	}
}

Server::Server(volatile sig_atomic_t &running) : running_(running), inactiveTimeout_(30), reactor_(NULL)
{
	std::cout << StringUtils::currentTime() << " webserver starting\n";

	{
		Config config(vhosts_, endpoints_, inactiveTimeout_);
	}

	reactor_ = new IOReactor(inactiveTimeout_);

	for (ListenEndpoints::const_iterator it = endpoints_.begin(); it != endpoints_.end(); ++it)
	{
		try { new ListeningSocket(it->first, *reactor_, endpoints_); }
		catch (const std::exception &e) { std::cerr << e.what() << "\n"; }
	}

	if (reactor_->empty())
	{
		delete reactor_;
		throw std::runtime_error(StringUtils::currentTime() + " [error] No listening sockets could be opened");
	}
}

void Server::run()
{
	while (running_)
		reactor_->waitAndDispatch(1000);
	std::cout << StringUtils::currentTime() << " webserver shutting down, sending responses to active clients and closing connections\n";
}

Server::~Server()
{
	delete reactor_;
	std::cout << StringUtils::currentTime() << " webserver stopped\n";
}