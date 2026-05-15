#pragma once

#include "config/Config.hpp"		// ListenEndpoints
#include "http/HttpRequest.hpp"		// HttpRequest
#include "http/HttpResponse.hpp"	// HttpResponse
#include "utils/NetworkUtils.hpp"	// Interface

namespace HttpPipeline
{

	void			resolve(HttpRequest &request, const ListenEndpoints &endpoints, const Interface &iface);

	int				prepareUploadRequest(HttpRequest &request);

	HttpResponse	buildResponse(const HttpRequest &request);
	HttpResponse	buildResponse(const HttpRequest &request, const std::string &raw);
	HttpResponse	uploadResponse(const HttpRequest &request);
	HttpResponse	errorResponse(const HttpRequest &request, HttpStatus::Code code);

	void			logRequest(const HttpRequest &request, const HttpResponse &response, const Interface &iface, size_t bytesSent);

};
