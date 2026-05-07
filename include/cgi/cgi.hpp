#pragma once

#include "http/HttpRequest.hpp"		// HttpRequest
#include "utils/NetworkUtils.hpp"	// Interface
#include "core/Pipe.hpp"			// Interface
#include "core/IPollable.hpp"		// IOReactor, IPollable
#include <string>					// string
#include <vector>					// vector

#include <ctime>					// size_t, time_t












class Client;

class CGIProcess : public IPollable
{

public:

	CGIProcess(IOReactor &reactor, Client &client);
	~CGIProcess();

	bool	running() const;
	void	onRead();
	void	onWrite();
	void	onTimeout();

	int		readFd() const;
	int		writeFd() const;

	Pipe		stdin_;
	Pipe		stdout_;
	pid_t		pid_;
	time_t		started_;
	std::string	output_buffer_;
	size_t		bytes_sent_;

private:

	CGIProcess(const CGIProcess &);
	CGIProcess &operator=(const CGIProcess &);

	Client *client_;

};

class CGIHandler
{

public:

	static void start(Client &client);
	static bool writeStdin(Client &client);
	static bool readStdout(Client &client);
	static void finish(Client &client);
	static void killProcess(Client &client);

private:

	static std::vector<std::string>	buildEnv(const HttpRequest &request, const Interface &iface);
	static std::vector<std::string>	buildArg(const std::string &filename, const Route &route);
	static std::string				resolveScript(const HttpRequest &request, std::vector<std::string> &env, const Route &route);

};
