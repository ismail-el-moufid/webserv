#include "http/HttpResponse.hpp"	// HttpResponse
#include "utils/StringUtils.hpp"	// toString

#include <sstream>					// ostringstream

void HttpResponse::init()
{
	headers_["Content-Type"]	= "text/plain";
	headers_["Connection"]		= "close";
	headers_["Content-Length"]	= "0";
}

HttpResponse::HttpResponse(void): status_(HttpStatus::OK)
{
	init();
}

void HttpResponse::setStatus(HttpStatus::Code status)							{ status_						= status; }
void HttpResponse::setHeader(const std::string &name, const std::string &value)	{ headers_[name]				= value; }
void HttpResponse::setBody(const std::string &body)								{ headers_["Content-Length"]	= StringUtils::toString(body.size()); body_ = body; }
void HttpResponse::setContentType(const std::string &type)						{ headers_["Content-Type"]		= type.empty() ? "text/plain" : type; }

std::string HttpResponse::serialize(void) const
{
	std::ostringstream ores;

	// HTTP/1.1 responses to 1.0 requests are valid as long as we avoid 1.1-only features
	ores << "HTTP/1.1 " << static_cast<int>(status_) << " " << HttpStatus::reasonPhrase(status_) << "\r\n";

	// write headers
	for (std::map<std::string, std::string>::const_iterator it = headers_.begin(); it != headers_.end(); ++it)
		ores << it->first << ": " << it->second << "\r\n";
	ores << "\r\n";

	// write body
	ores << body_;
	return ores.str();
}

HttpResponse HttpResponse::HttpErrorResponse(HttpStatus::Code status)
{
	std::ostringstream body;
	body << "<!doctype html><html lang=\"en-us\"><head><title>" << static_cast<int>(status) << " - " << HttpStatus::reasonPhrase(status) << "</title></head>"
		<< "<body><h1>" << static_cast<int>(status) << " " << HttpStatus::reasonPhrase(status) << "</h1></body></html>";

	HttpResponse res;
	res.setStatus(status);
	res.setContentType("text/html");
	res.setBody(body.str());
	return res;
}

HttpResponse HttpResponse::HttpResponseRedirect(HttpStatus::Code status, const std::string &location)
{
	std::ostringstream body;
	body << "<!doctype html><html lang=\"en-us\"><head><title>Redirect</title></head><body>redirecting to <a href=\"" << location << "\"></a></body></html>";

	HttpResponse res;
	res.setStatus(status);
	res.setHeader("Location", location);
	res.setContentType("text/html");
	res.setBody(body.str());
	return res;
}

void HttpResponse::reset(void)
{
	status_ = HttpStatus::OK;
	headers_.clear();
	init();
	body_.clear();
}

HttpStatus::Code	HttpResponse::statusCode()	const { return status_; }
size_t				HttpResponse::bodySize()	const { return body_.size(); }
