#pragma once

#include "http/HttpStatusCodes.hpp"	// Code
#include "config/Route.hpp"			// Route

#include <string>					// string
#include <map>						// map
#include <vector>					// vector

class VirtualHost
{

public:

	VirtualHost();

	static VirtualHost &applyDefaults(VirtualHost &virtualHost);

	void	addName(const std::string &name);
	void	addRoute(Route &route);
	void	addErrorPage(const std::string &code, const std::string &page);

	bool	hasName(const std::string &name) const;

	const std::vector<std::string>					&names() const;
	const std::vector<Route>						&routes() const;
	const std::map<HttpStatus::Code, std::string>	&errorPages() const;

private:

	std::vector<std::string>				names_;
	std::vector<Route>						routes_;
	std::map<HttpStatus::Code, std::string>	errorPages_;

};
