#pragma once

#include "utils/NetworkUtils.hpp"	// Interface
#include "http/HttpRequest.hpp"		// HttpRequest
#include "core/Pipe.hpp"			// Pipe
#include "core/IPollable.hpp"		// IOReactor, IPollable

#include <string>					// string
#include <vector>					// vector
#include <cstddef>					// size_t

class Client;

class CGIProcess : public IPollable
{

public:

	CGIProcess(IOReactor &reactor, Client &client);
	~CGIProcess();

	void	onRead();
	void	onWrite();
	void	onTimeout();

	int		readFd() const;
	int		writeFd() const;

	Pipe		stdinPipe;
	Pipe		stdoutPipe;
	pid_t		pid;
	std::string	outputBuffer;
	std::string	pendingBody;
	size_t		bodyOffset;

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
