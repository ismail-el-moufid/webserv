#pragma once

#include "config/VirtualHost.hpp"	// VirtualHost
#include "utils/NetworkUtils.hpp"	// Interface, InterfaceCompare





#include <cstddef>					// size_t
#include <ctime>					// time_t
#include <string>					// string
#include <vector>					// vector
#include <map>						// map
#include <deque>					// deque
#include <set>						// set
#include <fstream>					// ifstream

typedef std::deque<VirtualHost>																	VirtualHosts;
typedef std::map<Interface, std::vector<VirtualHost *>, NetworkUtils::InterfaceCompare>			ListenEndpoints;
typedef std::map<Interface, std::set<std::string>, NetworkUtils::InterfaceCompare>				InterfaceToServerNames;



class Config
{

public:

	class TokenStream
	{

	public:

		class Token
		{

		public:

			enum Type { DirectiveWord, DirectiveDelimiter, BlockOpen, BlockClose };

			Token();
			Token(const std::string &content);
			Token(Type type);

			std::string	content;
			Type		type;
			size_t		line;
			size_t		column;
		};

		TokenStream(const std::string &fileName);

		void				push(const Token &token);
		void				push(const std::string &word);
		void				push(Token::Type type);
		bool				done() const;
		size_t				getPosition() const;
		void				setPosition(size_t pos);
		const Token			&peek() const;
		const std::string	&expect(Token::Type type);
		void				expect(const std::string &value);
		bool				accept(Token::Type type);
		bool				accept(const std::string &value);
		void				skipBlock();
		void				skipDirective();

		size_t				currentTokenizationLine;
		size_t				currentTokenizationColumn;
		std::string			filePath;

	private:

		std::vector<Token>	tokens_;
		size_t				pos_;

	};

	Config(const std::string &filePath, VirtualHosts &hosts, ListenEndpoints &endpoints, time_t &timeout);
	Config(VirtualHosts &hosts, ListenEndpoints &endpoints, time_t &timeout);

private:

	void	parse(const std::string &filePath, VirtualHosts &hosts, ListenEndpoints &endpoints, time_t &timeout);

	void	tokenize(std::ifstream &configIfs);

	// Block parsers
	void	parseServerBlock(VirtualHosts &hosts, ListenEndpoints &endpoints, InterfaceToServerNames &registeredNames);
	void	parseLocationBlock(VirtualHost &virtualHost, Route &defaults);

	// Directive dispatchers
	bool	parseServerDirective(VirtualHost &virtualHost, Route &defaults);
	bool	parseLocationDirective(Route &route);

	// Directive handlers
	void	parseListen(std::vector<Interface> &endpoints);
	void	parseServerName(VirtualHost &virtualHost, Route &);
	void	parseServerErrorPage(VirtualHost &virtualHost, Route &defaults);
	void	parseCgi(Route &route);
	void	parseUpload(Route &route);
	void	parseRedirect(Route &route);
	void	parseLocationErrorPage(Route &route);
	void	parseRoot(Route &route);
	void	parseIndex(Route &route);
	void	parseAutoIndex(Route &route);
	void	parseMaxBodySize(Route &route);
	void	parseAllowedMethods(Route &route);

	std::string relativeToConfDir(const std::string &path) const;

	TokenStream tokenStream_;
	std::string confDir_;


};
