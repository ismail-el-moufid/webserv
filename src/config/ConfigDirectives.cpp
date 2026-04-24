#include "config/Config.hpp"
#include "config/VirtualHost.hpp"	// VirtualHost
#include "utils/NetworkUtils.hpp"	// Interface, resolve

#include <algorithm>				// find
#include <stdexcept>				// runtime_error

void Config::parseListen(std::vector<Interface> &endpoints)
{
	const std::string &address	= tokenStream_.expect(TokenStream::Token::DirectiveWord);
	const std::string &port		= tokenStream_.expect(TokenStream::Token::DirectiveWord);
	tokenStream_.expect(TokenStream::Token::DirectiveDelimiter);

	Interface endpoint;
	if (!NetworkUtils::resolve(address, port, endpoint))
		throw std::runtime_error("listen: could not resolve '" + address + ":" + port + "'");

	if (std::find(endpoints.begin(), endpoints.end(), endpoint) == endpoints.end())
		endpoints.push_back(endpoint);
}

void Config::parseServerName(VirtualHost &virtualHost, Route &)
{
	while (!tokenStream_.accept(TokenStream::Token::DirectiveDelimiter))
		virtualHost.addName(tokenStream_.expect(TokenStream::Token::DirectiveWord));
}

void Config::parseServerErrorPage(VirtualHost &virtualHost, Route &defaults)
{
	const std::string &code = tokenStream_.expect(TokenStream::Token::DirectiveWord);
	const std::string &page = tokenStream_.expect(TokenStream::Token::DirectiveWord);
	tokenStream_.expect(TokenStream::Token::DirectiveDelimiter);

	virtualHost.addErrorPage(code, page);
	defaults.addErrorPage(code, page);
}

void Config::parseCgi(Route &route)
{
	const std::string &extension	= tokenStream_.expect(TokenStream::Token::DirectiveWord);
	const std::string &path			= tokenStream_.expect(TokenStream::Token::DirectiveWord);
	tokenStream_.expect(TokenStream::Token::DirectiveDelimiter);

	route.addCgi(extension, path);
}

void Config::parseRedirect(Route &route)
{
	const std::string &code	= tokenStream_.expect(TokenStream::Token::DirectiveWord);
	const std::string &url	= tokenStream_.expect(TokenStream::Token::DirectiveWord);
	tokenStream_.expect(TokenStream::Token::DirectiveDelimiter);

	route.setRedirect(code, url);
}

void Config::parseLocationErrorPage(Route &route)
{
	const std::string &code = tokenStream_.expect(TokenStream::Token::DirectiveWord);
	const std::string &page = tokenStream_.expect(TokenStream::Token::DirectiveWord);
	tokenStream_.expect(TokenStream::Token::DirectiveDelimiter);
	
	route.addErrorPage(code, page);
}

void Config::parseUpload(Route &route)		{ route.setUpload(tokenStream_.expect(TokenStream::Token::DirectiveWord));		tokenStream_.expect(TokenStream::Token::DirectiveDelimiter); }
void Config::parseRoot(Route &route)		{ route.setRoot(tokenStream_.expect(TokenStream::Token::DirectiveWord));			tokenStream_.expect(TokenStream::Token::DirectiveDelimiter); }
void Config::parseAutoIndex(Route &route)	{ route.setAutoIndex(tokenStream_.expect(TokenStream::Token::DirectiveWord));		tokenStream_.expect(TokenStream::Token::DirectiveDelimiter); }
void Config::parseMaxBodySize(Route &route)	{ route.setMaxBodySize(tokenStream_.expect(TokenStream::Token::DirectiveWord));	tokenStream_.expect(TokenStream::Token::DirectiveDelimiter); }

void Config::parseMethods(Route &route)	{ route.clearMethods(); while (!tokenStream_.accept(TokenStream::Token::DirectiveDelimiter)) route.addMethod(tokenStream_.expect(TokenStream::Token::DirectiveWord)); }
void Config::parseIndex(Route &route)	{ while (!tokenStream_.accept(TokenStream::Token::DirectiveDelimiter)) route.addIndexFile(tokenStream_.expect(TokenStream::Token::DirectiveWord)); }
