#pragma once








#include <string>					// string














namespace HttpStatus
{

enum Code
{
	/*####### 2xx - Successful #######*/
	OK								= 200,
	Created							= 201,
	NoContent						= 204,

	/*####### 3xx - Redirection #######*/
	MovedPermanently				= 301,
	Found							= 302,
	SeeOther						= 303,
	TemporaryRedirect				= 307,
	PermanentRedirect				= 308,

	/*####### 4xx - Client Error #######*/
	BadRequest						= 400,
	Forbidden						= 403,
	NotFound						= 404,
	MethodNotAllowed				= 405,
	ContentTooLarge					= 413,
	URITooLong						= 414,
	IamATeapot						= 418,
	RequestHeaderFieldsTooLarge		= 431,

	/*####### 5xx - Server Error #######*/
	InternalServerError				= 500,
	NotImplemented					= 501,
	GatewayTimeout					= 504,
	HTTPVersionNotSupported			= 505,
};

	std::string	reasonPhrase(int code);
	bool		isRedirectCode(Code code);
	bool		isRedirectCode(const std::string &code);
	bool		isErrorCode(Code code);
	bool		isErrorCode(const std::string &code);
	Code		toCode(const std::string &code);
}
