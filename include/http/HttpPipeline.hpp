#pragma once

#include "config/Config.hpp"		// ListenEndpoints
#include "http/HttpRequest.hpp"		// HttpRequest
#include "http/HttpResponse.hpp"	// HttpResponse
#include "utils/NetworkUtils.hpp"	// Interface


















namespace HttpPipeline
{

	void			resolve(HttpRequest &request, const ListenEndpoints &endpoints, const Interface &iface);

	HttpResponse	buildResponse(const HttpRequest &request);

	void			logRequest(const HttpRequest &request, const HttpResponse &response, const Interface &iface, size_t bytesSent);

	HttpResponse	errorResponse(const HttpRequest &request, HttpStatus::Code code);
};
