#include "config/Route.hpp"			// Route
#include "http/HttpStatusCodes.hpp"	// Code, isRedirectCode, toCode
#include "utils/StringUtils.hpp"	// isAllDigits
#include <stdexcept>				// runtime_error
#include <cstdlib>					// NULL, size_t, strtoul

Route::Route() : maxBodySize_(0), redirect_(false), autoindex_(false) {}

Route &Route::applyDefaults(Route &route)
{
	route.methods_.push_back("GET");
	route.indexFiles_.push_back("welcome.html");
	route.root_ = HtmlDir;
	return route;
}

void Route::setMaxBodySize(const std::string &maxBodySize)
{
	if (!StringUtils::isAllDigits(maxBodySize))
		throw std::runtime_error("clientMaxBodySize must be a number");
	maxBodySize_ = strtoul(maxBodySize.c_str(), NULL, 10);
}
void	Route::setPath(const std::string &path)	{ path_ = StringUtils::normalizeSlashes(path); }
void	Route::setRoot(const std::string &root)	{ (root_ = StringUtils::normalizeSlashes(root)).erase(root_.find_last_not_of('/') + 1); }


void Route::addErrorPage(const std::string &code, const std::string &page)
{
	if (!HttpStatus::isErrorCode(code))
		throw std::runtime_error("Invalid error code");
	errorPages_[HttpStatus::toCode(code)] = page;
}

void Route::addMethod(const std::string &method)
{
	if (method != "GET" && method != "POST" && method != "DELETE")
		throw std::runtime_error("Invalid method: " + method);
	methods_.push_back(method);
}

void Route::setRedirect(const std::string &code, const std::string &page)
{
	if (!cgis_.empty())
		throw std::runtime_error("redirect and cgi are mutually exclusive");
	if (!upload_.empty())
		throw std::runtime_error("redirect and upload are mutually exclusive");
	if (!HttpStatus::isRedirectCode(code))
		throw std::runtime_error("Invalid redirect code");
	redirect_		= true;
	redirectCode_	= HttpStatus::toCode(code);
	redirectPage_	= page;
}

void Route::setUpload(const std::string &upload)
{
	if (redirect_)
		throw std::runtime_error("redirect and upload are mutually exclusive");
	upload_ = upload;
}

void Route::setAutoIndex(const std::string &autoindex)
{
	if (autoindex == "on")			autoindex_ = true;
	else if (autoindex == "off")	autoindex_ = false;
	else							throw std::runtime_error("autoIndex must be 'on' or 'off'");
}

void Route::addCgi(const std::string &extension, const std::string &path)
{
	if (redirect_)
		throw std::runtime_error("redirect and cgi are mutually exclusive");
	cgis_[extension] = path;
}

void	Route::addIndexFile(const std::string &indexFile) { indexFiles_.push_back(indexFile); }
void	Route::clearMethods() { methods_.clear(); }

const std::string								&Route::path()			const { return path_; }
size_t											Route::maxBodySize()	const { return maxBodySize_; }
const std::vector<std::string>					&Route::methods()		const { return methods_; };
const std::string								&Route::root()			const { return root_; }
bool											Route::redirected()		const { return redirect_; }
const std::string								&Route::redirectPage()	const { return redirectPage_; }
HttpStatus::Code								Route::redirectCode()	const { return redirectCode_; }
bool											Route::autoIndexed()	const { return autoindex_; }
const std::vector<std::string>					&Route::indexFiles()	const { return indexFiles_; }
const std::map<HttpStatus::Code, std::string>	&Route::errorPages()	const { return errorPages_; }
const std::map<std::string, std::string>		&Route::cgis()			const { return cgis_; }
const std::string								&Route::upload()		const { return upload_; }
