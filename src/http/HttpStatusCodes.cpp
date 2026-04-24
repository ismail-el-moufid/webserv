#include "http/HttpStatusCodes.hpp"

namespace HttpStatus
{

std::string reasonPhrase(int code)
{
	switch (code)
	{
	case 200:	return "OK";
	case 201:	return "Created";
	case 204:	return "No Content";
	case 301:	return "Moved Permanently";
	case 302:	return "Found";
	case 303:	return "See Other";
	case 307:	return "Temporary Redirect";
	case 308:	return "Permanent Redirect";
	case 400:	return "Bad Request";
	case 403:	return "Forbidden";
	case 404:	return "Not Found";
	case 405:	return "Method Not Allowed";
	case 413:	return "Content Too Large";
	case 414:	return "URI Too Long";
	case 418:	return "I'm a teapot";
	case 431:	return "Request Header Fields Too Large";
	case 500:	return "Internal Server Error";
	case 501:	return "Not Implemented";
	case 504:	return "Gateway Timeout";
	case 505:	return "HTTP Version Not Supported";
	default:	 return std::string();
	}
}

bool isRedirectCode(Code code)
{
	return code == MovedPermanently || code == Found || code == SeeOther || code == TemporaryRedirect || code == PermanentRedirect;
}

bool isRedirectCode(const std::string &code)
{
	return code == "301" || code == "302" || code == "303"
		|| code == "307" || code == "308";
}

bool isErrorCode(Code code)
{
	return code == BadRequest			|| code == Forbidden
		|| code == NotFound				|| code == MethodNotAllowed
		|| code == ContentTooLarge		|| code == URITooLong
		|| code == IamATeapot			|| code == RequestHeaderFieldsTooLarge
		|| code == InternalServerError	|| code == NotImplemented
		|| code == GatewayTimeout		|| code == HTTPVersionNotSupported;
}

bool isErrorCode(const std::string &code)
{
	return code == "400" || code == "403" || code == "404" || code == "405"
		|| code == "413" || code == "414" || code == "418" || code == "431"
		|| code == "500" || code == "501" || code == "504" || code == "505";
}

Code toCode(const std::string &code)
{
	return static_cast<Code>((code.at(0) - '0') * 100 + (code.at(1) - '0') * 10 + (code.at(2) - '0'));
}

}
