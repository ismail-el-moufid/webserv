#include "http/HttpResponse.hpp"	// HttpResponse
#include "utils/StringUtils.hpp"	// toString
#include "Defaults.hpp"				// SERVER_SOFTWARE

#include <sstream>					// ostringstream

void HttpResponse::init()
{
	headers_["Server"]			= SERVER_SOFTWARE;
	headers_["Content-Type"]	= "text/plain";
	headers_["Connection"]		= "close";
	headers_["Content-Length"]	= "0";
}

HttpResponse::HttpResponse(): status_(HttpStatus::OK)
{
	init();
}

void				HttpResponse::setStatus(HttpStatus::Code status)							{ status_						= status; }
void				HttpResponse::setHeader(const std::string &name, const std::string &value)	{ headers_[name]				= value; }
void				HttpResponse::setBody(const std::string &body)								{ headers_["Content-Length"]	= StringUtils::toString(body.size()); body_ = body; }
void				HttpResponse::setContentType(const std::string &type)						{ headers_["Content-Type"]		= type.empty() ? "text/plain" : type; }
void				HttpResponse::setFile(const std::string &path, size_t size)					{ filePath_ = path; headers_["Content-Length"] = StringUtils::toString(size); }

bool				HttpResponse::hasFile()												const	{ return !filePath_.empty(); }

const std::string	&HttpResponse::filePath()											const	{ return filePath_; }

void				HttpResponse::addCookie(const std::string &value)							{ cookies_.push_back(value); }

std::string HttpResponse::serialize() const
{
	std::ostringstream ores;
	ores << "HTTP/1.1 " << static_cast<int>(status_) << " " << HttpStatus::reasonPhrase(status_) << "\r\n";

	for (std::map<std::string, std::string>::const_iterator it = headers_.begin(); it != headers_.end(); ++it)
		ores << it->first << ": " << it->second << "\r\n";

	for (std::vector<std::string>::const_iterator it = cookies_.begin(); it != cookies_.end(); ++it)
		ores << "Set-Cookie: " << *it << "\r\n";

	ores << "\r\n";
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
	HttpResponse res;
	res.setStatus(status);
	res.setHeader("Location", location);
	res.setContentType("text/html");
	res.setBody("<!doctype html><html lang=\"en-us\"><head><title>Redirect</title></head>"
		"<body>redirecting to <a href=\"" + location + "\">" + location + "</a></body></html>");
	return res;
}

HttpResponse HttpResponse::HttpResponseBuilder(HttpStatus::Code code, const std::string &body, const std::string &contentType)
{
	HttpResponse res;
	res.setStatus(code);
	res.setContentType(contentType);
	res.setBody(body);
	return res;
}

HttpResponse HttpResponse::HttpJsonResponse(HttpStatus::Code code, const std::string &message)
{
	return HttpResponseBuilder(code, "{\"status\":\"" + message + "\"}", "application/json");
}

HttpResponse HttpResponse::HttpJsonErrorResponse(HttpStatus::Code code)
{
	std::string body = "{\"status\":\"error\",\"message\":\"" + HttpStatus::reasonPhrase(code) + "\"}";
	return HttpResponseBuilder(code, body, "application/json");
}

void HttpResponse::reset()
{
	status_ = HttpStatus::OK;
	headers_.clear();
	cookies_.clear();
	init();
	body_.clear();
	filePath_.clear();
}

HttpStatus::Code	HttpResponse::statusCode()	const { return status_; }
