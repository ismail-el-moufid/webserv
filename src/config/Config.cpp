#include "config/Config.hpp"		// Config, DefaultConfig, InterfaceToServerNames, ListenEndpoints, VirtualHosts and 6 more
#include "config/VirtualHost.hpp"	// VirtualHost
#include "utils/NetworkUtils.hpp"	// Interface, resolve
#include "utils/StringUtils.hpp"	// toString
#include "Defaults.hpp"				// DefaultConfig

#include <cstddef>					// size_t
#include <ctime>					// time_t
#include <iostream>					// cerr, endl, streamsize
#include <limits>					// numeric_limits
#include <limits.h>					// PATH_MAX
#include <stdexcept>				// runtime_error
#include <algorithm>				// find
#include <cstdlib>					// strtol
#include <string>					// string, rfind

typedef void (Config::*LocationDirectiveHandler)(Route &);
typedef void (Config::*ServerDirectiveHandler)(VirtualHost &, Route &);

Config::Config(const std::string &filePath, VirtualHosts &hosts, ListenEndpoints &endpoints, time_t &timeout) : tokenStream_(filePath)	{ parse(filePath, hosts, endpoints, timeout); }
Config::Config(VirtualHosts &hosts, ListenEndpoints &endpoints, time_t &timeout) : tokenStream_(DefaultConfig)							{ parse(DefaultConfig, hosts, endpoints, timeout); }

void Config::parse(const std::string &filePath, VirtualHosts &hosts, ListenEndpoints &endpoints, time_t &timeout)
{
	std::ifstream configIfs(filePath.c_str());
	size_t slash = filePath.rfind('/');
	confDir_ = (slash != std::string::npos) ? filePath.substr(0, slash) : ".";
	if (!configIfs)
		throw std::runtime_error(StringUtils::currentTime() + " [error] Failed to open config file: " + filePath);

	tokenize(configIfs);

	InterfaceToServerNames registeredNames;
	while (!tokenStream_.done())
	{
		if (tokenStream_.accept("timeout"))
		{
			const std::string val = tokenStream_.expect(TokenStream::Token::DirectiveWord);
			char *end;
			long parsed = std::strtol(val.c_str(), &end, 10);
			if (end == val.c_str() || *end != '\0' || parsed <= 0)
				throw std::runtime_error(StringUtils::currentTime() + " [error] Invalid timeout value: \"" + val + "\"");
			timeout = static_cast<time_t>(parsed);
			tokenStream_.expect(TokenStream::Token::DirectiveDelimiter);
		}
		else
			parseServerBlock(hosts, endpoints, registeredNames);
	}

	if (hosts.empty())
	{
		Interface endpoint;
		if (!NetworkUtils::resolve("localhost", "8080", endpoint))
			throw std::runtime_error(StringUtils::currentTime() + " [error] Failed to resolve default endpoint");
		VirtualHost virtualHost;
		hosts.push_back(VirtualHost::applyDefaults(virtualHost));
		endpoints[endpoint].push_back(&hosts.back());
	}
}

void Config::tokenize(std::ifstream &configIfs)
{
	static const std::string delimiters("{};\n\t ");
	int next;
	while ((next = configIfs.peek()) != EOF)
	{
		switch (next)
		{
		case '\n': ++tokenStream_.currentTokenizationLine; tokenStream_.currentTokenizationColumn = 1;															configIfs.ignore(); break;
		case ' ': case '\t': case '\r': ++tokenStream_.currentTokenizationColumn;																				configIfs.ignore(); break;
		case '{': tokenStream_.push(TokenStream::Token::BlockOpen); ++tokenStream_.currentTokenizationColumn;												configIfs.ignore(); break;
		case '}': tokenStream_.push(TokenStream::Token::BlockClose); ++tokenStream_.currentTokenizationColumn;											configIfs.ignore(); break;
		case ';': tokenStream_.push(TokenStream::Token::DirectiveDelimiter); ++tokenStream_.currentTokenizationColumn;									configIfs.ignore(); break;
		case '#': configIfs.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); tokenStream_.currentTokenizationColumn = 1; ++tokenStream_.currentTokenizationLine;	break;
		case '"':
			configIfs.ignore(); // skip opening quote
			{
				TokenStream::Token word;
				word.line	= tokenStream_.currentTokenizationLine;
				word.column	= tokenStream_.currentTokenizationColumn;
				while ((next = configIfs.peek()) != EOF && next != '"' && next != '\n')
				{
					word.content += (char)configIfs.get();
					++tokenStream_.currentTokenizationColumn;
				}
				if (next == '"')
					configIfs.ignore(); // skip closing quote
				tokenStream_.push(word);
			}
			break;
		default:
			TokenStream::Token word;
			word.line	= tokenStream_.currentTokenizationLine;
			word.column	= tokenStream_.currentTokenizationColumn;
			while (next != EOF && delimiters.find(next) == std::string::npos)
			{
				word.content += (char)configIfs.get();
				++tokenStream_.currentTokenizationColumn;
				next = configIfs.peek();
			}
			tokenStream_.push(word);
			break;
		}
	}
}

void Config::parseServerBlock(VirtualHosts &hosts, ListenEndpoints &endpoints, InterfaceToServerNames &registeredNames)
{
	tokenStream_.expect("server");
	tokenStream_.expect(TokenStream::Token::BlockOpen);

	Route					defaults;
	VirtualHost				virtualHost;
	std::vector<Interface>	currentServerBlockEndpoints;
	size_t					serverBlockStart = tokenStream_.getPosition();

	while (!tokenStream_.accept(TokenStream::Token::BlockClose))
	{
		if (tokenStream_.accept("location"))
		{
			tokenStream_.expect(TokenStream::Token::DirectiveWord);
			tokenStream_.skipBlock();
		}
		else if (tokenStream_.accept("listen"))
			parseListen(currentServerBlockEndpoints);
		else if (!parseServerDirective(virtualHost, defaults) && !parseLocationDirective(defaults))
			throw std::runtime_error(StringUtils::currentTime() + tokenStream_.filePath + ":" + StringUtils::toString(tokenStream_.peek().line) +
															":" + StringUtils::toString(tokenStream_.peek().column) +
															": error: unknown directive '" + tokenStream_.peek().content + "'");
	}

	if (currentServerBlockEndpoints.empty())
		throw std::runtime_error(StringUtils::currentTime() + " [error] Server block must have a listen directive");

	tokenStream_.setPosition(serverBlockStart);
	while (!tokenStream_.accept(TokenStream::Token::BlockClose))
	{
		if (tokenStream_.accept("location"))
			parseLocationBlock(virtualHost, defaults);
		else
			tokenStream_.skipDirective();
	}

	if (virtualHost.routes().empty())
		virtualHost.addRoute(defaults);

	hosts.push_back(virtualHost);
	const std::vector<std::string> &names = hosts.back().names();
	for (std::vector<Interface>::const_iterator it = currentServerBlockEndpoints.begin(); it != currentServerBlockEndpoints.end(); ++it)
	{
		endpoints[*it].push_back(&hosts.back());
		for (size_t n = 0; n < names.size(); ++n)
			if (!registeredNames[*it].insert(names[n]).second)
				std::cerr << "warning: conflicting serverName '" << names[n] << "' on same interface" << std::endl;
	}
}

void Config::parseLocationBlock(VirtualHost &virtualHost, Route &defaults)
{
	Route route(defaults);
	route.setPath(tokenStream_.expect(TokenStream::Token::DirectiveWord));
	tokenStream_.expect(TokenStream::Token::BlockOpen);

	while (!tokenStream_.accept(TokenStream::Token::BlockClose))
		if (!parseLocationDirective(route))
			throw std::runtime_error(StringUtils::currentTime() + tokenStream_.filePath + ":" + StringUtils::toString(tokenStream_.peek().line) +
															":" + StringUtils::toString(tokenStream_.peek().column) +
															": error: unknown directive '" + tokenStream_.peek().content + "'");

	virtualHost.addRoute(route);
}

bool Config::parseServerDirective(VirtualHost &virtualHost, Route &defaults)
{
	static std::map<std::string, ServerDirectiveHandler> handlers;
	if (handlers.empty())
	{
		handlers["serverName"]	= &Config::parseServerName;
		handlers["errorPage"]	= &Config::parseServerErrorPage;
	}

	std::map<std::string, ServerDirectiveHandler>::const_iterator handler = handlers.find(tokenStream_.peek().content);
	if (handler == handlers.end())
	{
		static std::vector<std::string> locationOnlyHandlers;
		if (locationOnlyHandlers.empty())
		{
			locationOnlyHandlers.push_back("cgi");
			locationOnlyHandlers.push_back("upload");
			locationOnlyHandlers.push_back("redirect");
		}
		if (std::find(locationOnlyHandlers.begin(), locationOnlyHandlers.end(), tokenStream_.peek().content) != locationOnlyHandlers.end())
			throw std::runtime_error(StringUtils::currentTime() + tokenStream_.filePath + ":" + StringUtils::toString(tokenStream_.peek().line) +
															":" + StringUtils::toString(tokenStream_.peek().column) +
															": error: directive '" + tokenStream_.peek().content + "' not allowed at server level");
		return false;
	}

	tokenStream_.expect(TokenStream::Token::DirectiveWord);
	(this->*handler->second)(virtualHost, defaults);
	return true;
}

bool Config::parseLocationDirective(Route &route)
{
	static std::map<std::string, LocationDirectiveHandler> handlers;
	if (handlers.empty())
	{
		handlers["cgi"]					= &Config::parseCgi;
		handlers["upload"]				= &Config::parseUpload;
		handlers["redirect"]			= &Config::parseRedirect;
		handlers["errorPage"]			= &Config::parseLocationErrorPage;
		handlers["root"]				= &Config::parseRoot;
		handlers["index"]				= &Config::parseIndex;
		handlers["autoIndex"]			= &Config::parseAutoIndex;
		handlers["clientMaxBodySize"]	= &Config::parseMaxBodySize;
		handlers["allowedMethods"]		= &Config::parseAllowedMethods;
	}
	std::map<std::string, LocationDirectiveHandler>::const_iterator handler = handlers.find(tokenStream_.peek().content);
	if (handler == handlers.end())
		return false;
	tokenStream_.expect(TokenStream::Token::DirectiveWord);
	(this->*handler->second)(route);
	return true;
}

std::string Config::relativeToConfDir(const std::string &path) const
{
	if (path.empty() || path[0] == '/')
		return path;
	std::string full = confDir_ + "/" + path;
	char resolved[PATH_MAX];
	if (realpath(full.c_str(), resolved))
		return resolved;
	return full;
}

