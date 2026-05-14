#include "Server.hpp"
#include <iostream>
#include <csignal>

static volatile sig_atomic_t g_running = 1;

static void onSignal(int) { g_running = 0; }

int main(int argc, char **argv)
{
	signal(SIGTERM,	onSignal);
	signal(SIGINT,	onSignal);
	signal(SIGPIPE,	SIG_IGN);

	try
	{
		if (argc == 1)
			Server(g_running).run();
		else if (argc == 2)
			Server(argv[1], g_running).run();
		else
		{
			std::cerr << "usage: webserv [config]" << std::endl;
			return 1;
		}
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
	return 0;
}