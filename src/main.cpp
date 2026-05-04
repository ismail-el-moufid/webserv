#include "Server.hpp"
#include <iostream>

int main(int argc, char **argv)
{
	try
	{
		if (argc == 1)
			Server().run();
		else if (argc == 2)
			Server(argv[1]).run();
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
