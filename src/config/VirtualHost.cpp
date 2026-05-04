#include "config/VirtualHost.hpp"	// VirtualHost
#include "http/HttpStatusCodes.hpp"	// HttpStatus Codes
#include "utils/StringUtils.hpp"	// toLower
#include "Defaults.hpp"				// HtmlDir

#include <string>					// string
#include <utility>					// make_pair, pair
#include <algorithm>				// find
#include <stdexcept>				// runtime_error

VirtualHost::VirtualHost() : errorPages_() {}

VirtualHost &VirtualHost::applyDefaults(VirtualHost &virtualHost)
{
	Route defaultRoute;

	virtualHost.routes_.push_back(Route::applyDefaults(defaultRoute));
	virtualHost.errorPages_.insert(std::make_pair(HttpStatus::BadRequest,						HtmlDir "/400.html"));
	virtualHost.errorPages_.insert(std::make_pair(HttpStatus::Forbidden,						HtmlDir "/403.html"));
	virtualHost.errorPages_.insert(std::make_pair(HttpStatus::NotFound,						HtmlDir "/404.html"));
	virtualHost.errorPages_.insert(std::make_pair(HttpStatus::MethodNotAllowed,				HtmlDir "/405.html"));
	virtualHost.errorPages_.insert(std::make_pair(HttpStatus::ContentTooLarge,				HtmlDir "/413.html"));
	virtualHost.errorPages_.insert(std::make_pair(HttpStatus::URITooLong,						HtmlDir "/414.html"));
	virtualHost.errorPages_.insert(std::make_pair(HttpStatus::IamATeapot,						HtmlDir "/418.html"));
	virtualHost.errorPages_.insert(std::make_pair(HttpStatus::RequestHeaderFieldsTooLarge,	HtmlDir "/431.html"));
	virtualHost.errorPages_.insert(std::make_pair(HttpStatus::InternalServerError,			HtmlDir "/500.html"));
	virtualHost.errorPages_.insert(std::make_pair(HttpStatus::NotImplemented,					HtmlDir "/501.html"));
	virtualHost.errorPages_.insert(std::make_pair(HttpStatus::GatewayTimeout,					HtmlDir "/504.html"));
	virtualHost.errorPages_.insert(std::make_pair(HttpStatus::HTTPVersionNotSupported,		HtmlDir "/505.html"));

	return virtualHost;
}

void VirtualHost::addName(const std::string &name)
{
	for (size_t i = 0; i < name.size(); ++i)
		if (!std::isalnum(name.at(i)) && name.at(i) != '.' && name.at(i) != '-')
			throw std::runtime_error(StringUtils::currentTime() + " [error] Invalid server name: " + name);
	names_.push_back(StringUtils::toLower(name));
}

void VirtualHost::addErrorPage(const std::string &code, const std::string &page)
{
	if (!HttpStatus::isErrorCode(code))
		throw std::runtime_error(StringUtils::currentTime() + " [error] Invalid error code");
	errorPages_[HttpStatus::toCode(code)] = page;
}

void VirtualHost::addRoute(Route &route)					{ routes_.push_back(route); }
bool VirtualHost::hasName(const std::string &name) const	{ return std::find(names_.begin(), names_.end(), StringUtils::toLower(name)) != names_.end(); }

const std::vector<std::string>							&VirtualHost::names()		const { return names_; }
const std::map<HttpStatus::Code, std::string>			&VirtualHost::errorPages()	const { return errorPages_; }
const std::vector<Route>								&VirtualHost::routes()		const { return routes_; }
