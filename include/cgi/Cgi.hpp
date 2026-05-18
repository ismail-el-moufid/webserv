#pragma once

#include "utils/NetworkUtils.hpp"	// Interface
#include "http/HttpRequest.hpp"		// HttpRequest
#include "core/Pipe.hpp"			// Pipe
#include "core/IPollable.hpp"		// IOReactor, IPollable

#include <string>					// string
#include <vector>					// vector
#include <map>						// map

class Client;

class CGIProcess : public IPollable
{

public:

	CGIProcess(IOReactor &reactor, Client &client);
	~CGIProcess();

	int		readFd() const;
	int		writeFd() const;

	void	onRead();
	void	onWrite();
	void	onTimeout();


	Pipe		stdinPipe;
	Pipe		stdoutPipe;
	pid_t		pid;

	std::string	outputBuffer;

	enum sidSource
	{ 
		COOKIE,
		GENERATED
	};
	
	// Session ID and preloaded data for this CGI process
	std::string							sid;
	sidSource							sidSrc;
	std::map<std::string, std::string>	sessionData;

private:

	CGIProcess(const CGIProcess &);
	CGIProcess &operator=(const CGIProcess &);

	Client *client_;

};

class CGIHandler
{

public:

	static int	start(Client &client);
	static bool	writeStdin(Client &client);
	static bool	readStdout(Client &client);
	static void	finish(Client &client);
	static void	killProcess(Client &client, int code);

private:

	static std::vector<std::string>	buildEnv(const HttpRequest &request, const Interface &listeningIface, const Interface &clientIface, std::string &sid, CGIProcess::sidSource &sidSrc, std::map<std::string, std::string> &sessionData);
	static std::vector<std::string>	buildArg(const std::string &filename, const Route &route);
	static std::string				resolveScript(const HttpRequest &request, std::vector<std::string> &env, const Route &route);

};
