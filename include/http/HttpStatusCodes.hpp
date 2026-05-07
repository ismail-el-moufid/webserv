#pragma once








#include <string>					// string














namespace HttpStatus
{

	enum Code
	{
		Continue						= 100,
		SwitchingProtocols				= 101,
		Processing						= 102,
		EarlyHints						= 103,

		OK								= 200,
		Created							= 201,
		Accepted						= 202,
		NonAuthoritativeInformation		= 203,
		NoContent						= 204,
		ResetContent					= 205,
		PartialContent					= 206,
		MultiStatus						= 207,
		AlreadyReported					= 208,
		IMUsed							= 226,

		MultipleChoices					= 300,
		MovedPermanently				= 301,
		Found							= 302,
		SeeOther						= 303,
		NotModified						= 304,
		TemporaryRedirect				= 307,
		PermanentRedirect				= 308,

		BadRequest						= 400,
		Unauthorized					= 401,
		PaymentRequired					= 402,
		Forbidden						= 403,
		NotFound						= 404,
		MethodNotAllowed				= 405,
		NotAcceptable					= 406,
		ProxyAuthenticationRequired		= 407,
		RequestTimeout					= 408,
		Conflict						= 409,
		Gone							= 410,
		LengthRequired					= 411,
		PreconditionFailed				= 412,
		ContentTooLarge					= 413,
		URITooLong						= 414,
		UnsupportedMediaType			= 415,
		RangeNotSatisfiable				= 416,
		ExpectationFailed				= 417,
		IamATeapot						= 418,
		MisdirectedRequest				= 421,
		UnprocessableContent			= 422,
		Locked							= 423,
		FailedDependency				= 424,
		TooEarly						= 425,
		UpgradeRequired					= 426,
		PreconditionRequired			= 428,
		TooManyRequests					= 429,
		RequestHeaderFieldsTooLarge		= 431,
		UnavailableForLegalReasons		= 451,

		InternalServerError				= 500,
		NotImplemented					= 501,
		BadGateway						= 502,
		ServiceUnavailable				= 503,
		GatewayTimeout					= 504,
		HTTPVersionNotSupported			= 505,
		VariantAlsoNegotiates			= 506,
		InsufficientStorage				= 507,
		LoopDetected					= 508,
		NetworkAuthenticationRequired	= 511
	};

	std::string	reasonPhrase(Code code);
	bool		isValidCode(const std::string &code);
	bool		isRedirectCode(Code code);
	bool		isRedirectCode(const std::string &code);
	bool		isErrorCode(Code code);
	bool		isErrorCode(const std::string &code);
	Code		toCode(const std::string &code);
}
