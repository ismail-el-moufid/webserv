#include "cgi/Cgi.hpp"
#include "config/Route.hpp"
#include "core/Client.hpp"
#include "core/IOReactor.hpp"
#include "http/HttpPipeline.hpp"
#include "http/HttpStatusCodes.hpp"
#include "http/Session.hpp"
#include "utils/StringUtils.hpp"
#include "Defaults.hpp"

#include <signal.h>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

std::string CGIHandler::resolveScript(const HttpRequest &request, std::vector<std::string> &env, const Route &route)
{
	const std::string &path_part = request.uri().path;

	env.push_back("QUERY_STRING=" + request.uri().query);

	// Strip path-info suffix and extract path-info env vars.
	std::string script_name = path_part;
	size_t dot = path_part.find('.');
	if (dot != std::string::npos)
	{
		size_t path_info_slash = path_part.find('/', dot);
		if (path_info_slash != std::string::npos)
		{
			script_name				= path_part.substr(0, path_info_slash);
			std::string path_info	= path_part.substr(path_info_slash);
			env.push_back("PATH_INFO=" + path_info);
			env.push_back("PATH_TRANSLATED=" + route.root() + path_info);
		}
	}

	std::string filename = route.root() + script_name;
	env.push_back("SCRIPT_NAME=" + script_name);
	env.push_back("SCRIPT_FILENAME=" + filename);
	return filename;
}

std::vector<std::string> CGIHandler::buildEnv(const HttpRequest &request, const Interface &listeningIface, const Interface &clientIface, std::string &sid, CGIProcess::sidSource &sidSrc, std::map<std::string, std::string> &sessionData)
{
	std::vector<std::string> env;
	env.push_back("REQUEST_METHOD=" + request.method());
	env.push_back("SERVER_PROTOCOL=" + request.version());

	const std::map<std::string, std::string> &headers = request.headers();
	std::map<std::string,std::string>::const_iterator i;
	for(i = headers.begin(); i != headers.end(); i++)
	{
		std::string	key		= i->first;
		std::string	value	= i->second;

		for(size_t idx = 0; idx < key.length(); idx++)
		{
			key[idx] = std::toupper(key[idx]);
			if(key[idx] == '-')
				key[idx] = '_';
		}
		if(key == "CONTENT_LENGTH" || key == "CONTENT_TYPE")
			env.push_back(key + "=" + value);
		else
			env.push_back("HTTP_" + key + "=" + value);
	}
	env.push_back("SERVER_SOFTWARE=" SERVER_SOFTWARE);
	env.push_back("GATEWAY_INTERFACE=CGI/1.1");
	env.push_back("REDIRECT_STATUS=200");
	env.push_back("PATH=/usr/bin:/bin");

	std::string serverName, serverIp, serverPort, clientIp, clientPort;

	NetworkUtils::extractIPPort(listeningIface, serverIp, serverPort);
	NetworkUtils::extractIPPort(clientIface, clientIp, clientPort);
	if (!request.host().empty())
		serverName = request.host();
	else if (request.vhost && !request.vhost->names().empty())
		serverName = request.vhost->names().front();
	else
		serverName = serverIp;

	env.push_back("SERVER_NAME=" + serverName);
	env.push_back("REMOTE_ADDR=" + clientIp);
	env.push_back("SERVER_PORT=" + serverPort);
	env.push_back("REQUEST_URI=" + request.uri().uri);


	std::map<std::string, std::string>::const_iterator cit = headers.find("cookie");
	if (cit != headers.end())
	{
		sid = Session::parseSid(cit->second);
		sidSrc = CGIProcess::COOKIE;
	}

	sessionData = Session::load(sid);
	if (sid.empty() || sessionData.empty())
	{
		sid		= Session::generate();
		sidSrc	= CGIProcess::GENERATED;
	}

	for (std::map<std::string, std::string>::const_iterator it = sessionData.begin(); it != sessionData.end(); ++it)
	{
		std::string key = it->first;
		for (size_t idx = 0; idx < key.size(); ++idx)
			key[idx] = std::toupper(static_cast<unsigned char>(key[idx]));
		env.push_back("SESSION_" + key + "=" + it->second);
	}

	return env;
}

std::vector<std::string> CGIHandler::buildArg(const std::string &filename, const Route &route)
{
	std::vector<std::string>	arg;
	size_t						dot_pos = filename.rfind('.');
	if (dot_pos == std::string::npos)
		return arg;

	std::string extention = filename.substr(dot_pos);
	std::map<std::string, std::string>::const_iterator it = route.cgis().find(extention);
	if (it == route.cgis().end())
		return arg;

	arg.push_back(it->second);
	arg.push_back(filename);

	return arg;
}
int CGIHandler::start(Client &client)
{
	char **arguments;
	char **envm;
	pid_t pid;

	CGIProcess	&cgi = *(client.cgi);
	std::string	filename;

	// Build the base environment
	std::vector<std::string> environ = buildEnv(client.request, client.listeningIface, client.clientIface, cgi.sid, cgi.sidSrc, cgi.sessionData);

	// Resolve the script and finalize the environment BEFORE making pointers
	filename = resolveScript(client.request, environ, *client.request.route);

	// Create a persistent local variable for the arguments so the strings don't get destroyed!
	std::vector<std::string> args_str = buildArg(filename, *client.request.route);
	if (args_str.empty())
		return HttpStatus::InternalServerError;

	// Now it is safe to create the C-string arrays
	std::vector<const char *> env = StringUtils::toNullTerminatedCStrings(environ);
	std::vector<const char *> arg = StringUtils::toNullTerminatedCStrings(args_str);

	arguments	= const_cast<char**>(&arg[0]);
	envm		= const_cast<char**>(&env[0]);

	// Check if script exists and interpreter is executable before forking
	if (access(filename.c_str(), F_OK) == -1 || access(arguments[0], F_OK) == -1)
		return HttpStatus::NotFound;
	if (access(arguments[0], X_OK) == -1)
		return HttpStatus::Forbidden;

	switch (pid = fork())
	{
	case -1:
		return HttpStatus::InternalServerError;
	case 0:
		// Child Process
		dup2(cgi.stdinPipe.readFd(), STDIN_FILENO);
		dup2(cgi.stdoutPipe.writeFd(), STDOUT_FILENO);
		// restore pipe fd flags to default ( O_nonblock was set in the parent )
		fcntl(STDOUT_FILENO, F_SETFL, fcntl(STDOUT_FILENO, F_GETFL) & ~O_NONBLOCK);
		if (chdir(filename.substr(0, filename.rfind('/')).c_str()) != -1) // Change to the script's directory to support relative includes and such
			execve(arguments[0], arguments, envm);
		close(cgi.stdinPipe.readFd());
		close(cgi.stdoutPipe.writeFd());
		exit(1);
	default:
		// Parent Process
		cgi.stdinPipe.closeRead();
		cgi.stdoutPipe.closeWrite();
		cgi.pid = pid;
	}
	return 0;
}

static std::string applySession(std::string &raw, const std::string &sid, std::map<std::string, std::string> &sessionData);

void CGIHandler::finish(Client &client)
{
	int status;
	CGIProcess &cgi = *client.cgi;

	waitpid(cgi.pid, &status, 0);
	if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
	{
		std::string cookie = applySession(cgi.outputBuffer, cgi.sid, cgi.sessionData);
		client.response = HttpPipeline::buildResponse(client.request, cgi.outputBuffer);
		if (!cookie.empty() && cgi.sidSrc == CGIProcess::GENERATED)
			client.response.addCookie(cookie);
	}
	else
		client.response = HttpPipeline::errorResponse(client.request, HttpStatus::InternalServerError);
}

void CGIHandler::killProcess(Client &client, int code)
{
	int status;

	kill(client.cgi->pid, SIGKILL);
	waitpid(client.cgi->pid, &status, 0);
	if (code != -1)
		client.response = HttpPipeline::errorResponse(client.request, HttpStatus::Code(code));
}

bool CGIHandler::writeStdin(Client &client)
{
	int result = client.request.drainBody();
	if (result == -1)
	{
		client.response = HttpPipeline::errorResponse(client.request, HttpStatus::InternalServerError);
		return true;
	}
	return result == 1 && client.request.complete();
}

bool CGIHandler::readStdout(Client &client)
{
	ssize_t bytes;
	CGIProcess &cgi = *client.cgi;
	char buffer[4096];

	bytes = read(cgi.stdoutPipe.readFd(), buffer, 4096);
	if(bytes > 0)
		cgi.outputBuffer.append(buffer, bytes);
	else if(bytes == 0)
		return true;
	
	return false;
}

static std::string applySession(std::string &raw, const std::string &sid, std::map<std::string, std::string> &sessionData)
{
	// Find the header block
	size_t headerEnd = raw.find("\r\n\r\n");
	size_t step = 4;
	if (headerEnd == std::string::npos)
	{
		headerEnd = raw.find("\n\n");
		step = 2;
	}
	if (headerEnd == std::string::npos)
		return "";

	std::string headers	= raw.substr(0, headerEnd);
	std::string body	= raw.substr(headerEnd + step);

	std::map<std::string, std::string> mutations;
	std::string cleaned;

	size_t pos = 0;
	while (pos <= headers.size())
	{
		size_t		nl		= headers.find('\n', pos);
		std::string	line	= (nl == std::string::npos)
							? headers.substr(pos)
							: headers.substr(pos, nl - pos + 1);

		std::string trimmed = line;
		if (!trimmed.empty() && trimmed[trimmed.size() - 1] == '\n')
			trimmed.erase(trimmed.size() - 1);

		if (!trimmed.empty() && trimmed[trimmed.size() - 1] == '\r')
			trimmed.erase(trimmed.size() - 1);

		size_t colon = trimmed.find(':');
		if (colon != std::string::npos && trimmed.substr(0, colon) == "X-Set-Session")
		{
			std::string val = StringUtils::trim(trimmed.substr(colon + 1));
			size_t eq = val.find('=');
			if (eq != std::string::npos)
			{
				std::string key		= val.substr(0, eq);
				std::string value	= val.substr(eq + 1);

				mutations[key] = value;
			}
		}
		else
			cleaned += line;

		if (nl == std::string::npos)
			break;
		pos = nl + 1;
	}

	raw = cleaned + (step == 4 ? "\r\n\r\n" : "\n\n") + body;

	for (std::map<std::string, std::string>::const_iterator it = mutations.begin(); it != mutations.end(); ++it)
		sessionData[it->first] = it->second;
	Session::save(sid, sessionData);
	return "sid=" + sid + "; Path=/";
}

#define CGI_READ_ONLY	POLLIN

CGIProcess::CGIProcess(IOReactor &reactor, Client &client) : IPollable(reactor), pid(-1), client_(&client) { }

CGIProcess::~CGIProcess() { reactor_.remove(*this); }

void CGIProcess::onWrite()
{
	if (CGIHandler::writeStdin(*client_))
	{
		reactor_.mod(*this, CGI_READ_ONLY);
		stdinPipe.closeWrite();
	}
}

void CGIProcess::onRead()
{
	updateActivity();
	if (CGIHandler::readStdout(*client_))
	{
		CGIHandler::finish(*client_);
		reactor_.remove(*this);
		client_->onCgiComplete();
	}
}

void CGIProcess::onTimeout()
{
	CGIHandler::killProcess(*client_, HttpStatus::GatewayTimeout);
	reactor_.remove(*this);
	client_->onCgiComplete();
}

int CGIProcess::readFd()	const { return stdoutPipe.readFd(); }
int CGIProcess::writeFd()	const { return stdinPipe.writeFd(); }
