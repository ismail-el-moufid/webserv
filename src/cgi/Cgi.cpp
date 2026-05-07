#include "cgi/Cgi.hpp"
#include "core/Client.hpp"
#include "core/IOReactor.hpp"
#include "http/HttpPipeline.hpp"
#include "utils/StringUtils.hpp"
#include "Defaults.hpp"

#include <signal.h>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

std::string CGIHandler::resolveScript(const HttpRequest &request, std::vector<std::string> &env, const Route &route)
{
	std::string uri = request.uri();
	size_t qm_pos = uri.find('?');
	std::string path_part;
	std::string filename;

	if (qm_pos != std::string::npos)
	{
		path_part = uri.substr(0, qm_pos);
		env.push_back("QUERY_STRING=" + uri.substr(qm_pos + 1));
	}
	else
	{
		path_part = uri;
		env.push_back("QUERY_STRING=");
	}
	std::string script_name = path_part;
	size_t dot_pos = path_part.find('.');
	if (dot_pos != std::string::npos)
	{
		size_t path_info_slash = path_part.find('/', dot_pos);

		if (path_info_slash != std::string::npos)
		{
			script_name = path_part.substr(0, path_info_slash);
			std::string path_info = path_part.substr(path_info_slash);
			env.push_back("PATH_INFO=" + path_info);
			env.push_back("PATH_TRANSLATED=" + route.root() + path_info);
		}
	}
	filename = route.root() + script_name;
	env.push_back("SCRIPT_NAME=" + script_name);
	env.push_back("SCRIPT_FILENAME=" + filename);
	return filename;
}
std::vector<std::string> CGIHandler::buildEnv(const HttpRequest &request, const Interface &iface)
{
	std::vector<std::string> env;
	env.push_back("REQUEST_METHOD=" + request.method());
	env.push_back("SERVER_PROTOCOL=" + request.version());
	std::string uri = request.uri();
	const std::map<std::string, std::string> headers = request.headers();
	std::map<std::string,std::string>::const_iterator i;
	for(i = headers.begin(); i != headers.end(); i++)
	{
		std::string key = i->first;
		std::string value = i->second;
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
	env.push_back("SERVER_SOFTWARE="SERVER_SOFTWARE);
	env.push_back("GATEWAY_INTERFACE=CGI/1.1");
	env.push_back("REDIRECT_STATUS=200");
	std::string ip, port, serverName;
	NetworkUtils::extractIPPort(iface, ip, port);
	if (!request.host().empty())
		serverName = request.host();
	else if (request.vhost && !request.vhost->names().empty())
		serverName = request.vhost->names().front();
	else
		serverName = ip;
	env.push_back("SERVER_NAME=" + serverName);
	env.push_back("REMOTE_ADDR=" + ip);
	env.push_back("SERVER_PORT=" + port);
	env.push_back("REQUEST_URI=" + request.uri());
	return env;
}

std::vector<std::string> CGIHandler::buildArg(const std::string &filename, const Route &route)
{
	size_t dot_pos = filename.rfind('.');
	std::vector<std::string> arg;
	std::string extention = filename.substr(dot_pos);
	std::map<std::string, std::string>::const_iterator it = route.cgis().find(extention);
	arg.push_back(it->second);
	arg.push_back(filename);
	return(arg);
}

void CGIHandler::start(Client &client)
{
	char **argumnets;
	char **envm;
	pid_t pid;

	CGIProcess &cgi = *(client.cgi);
	std::string filename;
	
	// Build the base environment
	std::vector<std::string> environ = buildEnv(client.request, client.iface);
	
	// Resolve the script and finalize the environment BEFORE making pointers
	filename = resolveScript(client.request, environ, *client.request.route);
	
	// Create a persistent local variable for the arguments so the strings don't get destroyed!
	std::vector<std::string> args_str = buildArg(filename, *client.request.route);

	// Now it is safe to create the C-string arrays
	std::vector<const char *> env = StringUtils::toNullTerminatedCStrings(environ);
	std::vector<const char *> arg = StringUtils::toNullTerminatedCStrings(args_str);

	argumnets = const_cast<char**>(&arg[0]);
	envm = const_cast<char**>(&env[0]);

	cgi.started_ = time(NULL);

	// Check if script exists before forking
	if (access(filename.c_str(), F_OK) == -1)
	{
		cgi.stdin_.closeRead();
		cgi.stdin_.closeWrite();
		cgi.stdout_.closeWrite();
		return;
	}

	switch (pid = fork())
	{
	case -1:
		client.clearCgi();
		client.response = HttpPipeline::errorResponse(client.request, HttpStatus::InternalServerError);
		return ;
	case 0:
		// Child Process
		dup2(cgi.stdin_.readFd(), STDIN_FILENO);
		dup2(cgi.stdout_.writeFd(), STDOUT_FILENO);
		// clear the O_NONBLOCK the pipe inherited
		fcntl(STDOUT_FILENO, F_SETFL, fcntl(STDOUT_FILENO, F_GETFL) & ~O_NONBLOCK);
		execve(argumnets[0], argumnets, envm);
		exit(1);
		break ;
	default:
		// Parent Process
		cgi.stdin_.closeRead();
		cgi.stdout_.closeWrite();
		cgi.pid_ = pid;
	}
}

void CGIHandler::finish(Client &client)
{
	int status;
	CGIProcess &cgi = *client.cgi;

	if (cgi.pid_ == -1) // We aborted early because the file didn't exist
	{
		client.response = HttpPipeline::errorResponse(client.request, HttpStatus::NotFound);
		return;
	}

	waitpid(cgi.pid_, &status, 0);
	if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
		client.response = HttpPipeline::buildResponseFromRaw(client.request, cgi.output_buffer_);
	else
		client.response = HttpPipeline::errorResponse(client.request, HttpStatus::InternalServerError);
}

void CGIHandler::killProcess(Client &client)
{
	int status;
	CGIProcess &cgi = *client.cgi;

	kill(cgi.pid_, SIGKILL);
	waitpid(cgi.pid_, &status, 0);
	client.response = HttpPipeline::errorResponse(client.request, HttpStatus::GatewayTimeout);
}


bool CGIHandler::writeStdin(Client &client)
{
	ssize_t written;

	CGIProcess &cgi = *client.cgi;
	const HttpRequest &request = client.request;

	if(cgi.bytes_sent_ >= request.contentLength())
		return true;

	written = write(cgi.stdin_.writeFd(), request.body().c_str() + cgi.bytes_sent_ , request.contentLength() - cgi.bytes_sent_);
	if(written > 0)
		cgi.bytes_sent_ += written;
	else if (written == -1)
		client.response = HttpPipeline::errorResponse(client.request, HttpStatus::InternalServerError);

	return false;
}

bool CGIHandler::readStdout(Client &client)
{
	ssize_t bytes;
	CGIProcess &cgi = *client.cgi;
	char buffer[4096];

	bytes = read(cgi.stdout_.readFd(), buffer, 4096);
	if(bytes > 0)
		cgi.output_buffer_.append(buffer, bytes);
	else if(bytes == 0)
	{
		return true;
	}
	else if (bytes == -1)
		client.response = HttpPipeline::errorResponse(client.request, HttpStatus::InternalServerError);

	return false;
}

#define CGI_READ_ONLY	POLLIN

CGIProcess::CGIProcess(IOReactor &reactor, Client &client) : IPollable(reactor), pid_(-1), started_(0), bytes_sent_(0), client_(&client) { }

CGIProcess::~CGIProcess() { reactor_.remove(*this); }

void CGIProcess::onWrite()
{
	if (CGIHandler::writeStdin(*client_))
	{
		reactor_.mod(*this, CGI_READ_ONLY);
		stdin_.closeWrite();
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
		client_->clearCgi();
	}
}

void CGIProcess::onTimeout()
{
	CGIHandler::killProcess(*client_);
	reactor_.remove(*this);
	client_->onCgiComplete();
	client_->clearCgi();
}

int		CGIProcess::readFd()	const { return stdout_.readFd(); }
int		CGIProcess::writeFd()	const { return stdin_.writeFd(); }
bool	CGIProcess::running()	const { return pid_ != -1; }
