#include "http/HttpStatusCodes.hpp"

namespace HttpStatus
{

	std::string reasonPhrase(Code code)
{
	switch (code)
	{
	// 1xx
	case Continue:						return "Continue";
	case SwitchingProtocols:			return "Switching Protocols";
	case Processing:					return "Processing";
	case EarlyHints:					return "Early Hints";
	// 2xx
	case OK:							return "OK";
	case Created:						return "Created";
	case Accepted:						return "Accepted";
	case NonAuthoritativeInformation:	return "Non-Authoritative Information";
	case NoContent:						return "No Content";
	case ResetContent:					return "Reset Content";
	case PartialContent:				return "Partial Content";
	case MultiStatus:					return "Multi-Status";
	case AlreadyReported:				return "Already Reported";
	case IMUsed:						return "IM Used";
	// 3xx
	case MultipleChoices:				return "Multiple Choices";
	case MovedPermanently:				return "Moved Permanently";
	case Found:							return "Found";
	case SeeOther:						return "See Other";
	case NotModified:					return "Not Modified";
	case TemporaryRedirect:				return "Temporary Redirect";
	case PermanentRedirect:				return "Permanent Redirect";
	// 4xx
	case BadRequest:					return "Bad Request";
	case Unauthorized:					return "Unauthorized";
	case PaymentRequired:				return "Payment Required";
	case Forbidden:						return "Forbidden";
	case NotFound:						return "Not Found";
	case MethodNotAllowed:				return "Method Not Allowed";
	case NotAcceptable:					return "Not Acceptable";
	case ProxyAuthenticationRequired:	return "Proxy Authentication Required";
	case RequestTimeout:				return "Request Timeout";
	case Conflict:						return "Conflict";
	case Gone:							return "Gone";
	case LengthRequired:				return "Length Required";
	case PreconditionFailed:			return "Precondition Failed";
	case ContentTooLarge:				return "Content Too Large";
	case URITooLong:					return "URI Too Long";
	case UnsupportedMediaType:			return "Unsupported Media Type";
	case RangeNotSatisfiable:			return "Range Not Satisfiable";
	case ExpectationFailed:				return "Expectation Failed";
	case IamATeapot:					return "I'm a teapot";
	case MisdirectedRequest:			return "Misdirected Request";
	case UnprocessableContent:			return "Unprocessable Content";
	case Locked:						return "Locked";
	case FailedDependency:				return "Failed Dependency";
	case TooEarly:						return "Too Early";
	case UpgradeRequired:				return "Upgrade Required";
	case PreconditionRequired:			return "Precondition Required";
	case TooManyRequests:				return "Too Many Requests";
	case RequestHeaderFieldsTooLarge:	return "Request Header Fields Too Large";
	case UnavailableForLegalReasons:	return "Unavailable For Legal Reasons";
	// 5xx
	case InternalServerError:			return "Internal Server Error";
	case NotImplemented:				return "Not Implemented";
	case BadGateway:					return "Bad Gateway";
	case ServiceUnavailable:			return "Service Unavailable";
	case GatewayTimeout:				return "Gateway Timeout";
	case HTTPVersionNotSupported:		return "HTTP Version Not Supported";
	case VariantAlsoNegotiates:			return "Variant Also Negotiates";
	case InsufficientStorage:			return "Insufficient Storage";
	case LoopDetected:					return "Loop Detected";
	case NetworkAuthenticationRequired:	return "Network Authentication Required";
	default:							return std::string();
	}
}

bool isRedirectCode(Code code)
{
	return code == MovedPermanently || code == Found || code == SeeOther
		|| code == TemporaryRedirect || code == PermanentRedirect;
}

bool isRedirectCode(const std::string &code)
{
	return code == "301" || code == "302" || code == "303"
		|| code == "307" || code == "308";
}

bool isErrorCode(Code code)
{
	return static_cast<int>(code) >= 400 && static_cast<int>(code) <= 599;
}

bool isErrorCode(const std::string &code)
{
	return code.size() == 3 && (code[0] == '4' || code[0] == '5');
}

bool isValidCode(const std::string &code)
{
	return code == "100" || code == "101" || code == "102" || code == "103"

		|| code == "200" || code == "201" || code == "202" || code == "203"
		|| code == "204" || code == "205" || code == "206" || code == "207"
		|| code == "208" || code == "226"

		|| code == "300" || code == "301" || code == "302" || code == "303"
		|| code == "304" || code == "307" || code == "308"

		|| code == "400" || code == "401" || code == "402" || code == "403"
		|| code == "404" || code == "405" || code == "406" || code == "407"
		|| code == "408" || code == "409" || code == "410" || code == "411"
		|| code == "412" || code == "413" || code == "414" || code == "415"
		|| code == "416" || code == "417" || code == "418" || code == "421"
		|| code == "422" || code == "423" || code == "424" || code == "425"
		|| code == "426" || code == "428" || code == "429" || code == "431"
		|| code == "451"

		|| code == "500" || code == "501" || code == "502" || code == "503"
		|| code == "504" || code == "505" || code == "506" || code == "507"
		|| code == "508" || code == "511";
}

Code toCode(const std::string &code)
{
	return static_cast<Code>((code.at(0) - '0') * 100 + (code.at(1) - '0') * 10 + (code.at(2) - '0'));
}

}
