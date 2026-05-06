#pragma once

#include "config/VirtualHost.hpp"	// VirtualHost
#include "http/HttpStatusCodes.hpp"	// Code





#include <string>					// string
#include <map>						// map







#define VALID 0
#define NEED_MORE_DATA 0




class HttpRequest
{

public:

	HttpRequest();

	void parse(const std::string &rawBytes);

	void setMaxBodySize(size_t maxBodySize);

	void reset();

	bool										complete() const;
	bool										headersComplete() const;
	const std::string							&method() const;
	const std::string							&uri() const;
	const std::string							&version() const;
	const std::map<std::string, std::string>	&headers() const;
	const std::string							&host() const;
	const std::string							&body() const;
	size_t										contentLength() const;
	bool										hasCgi() const;
	bool										erroneous() const;
	bool										connectionClose() const;
	HttpStatus::Code							errorCode() const;

	// config context
	const VirtualHost	*vhost;
	const Route			*route;

private:

	bool parseRequestLine();

	bool parseHeaders();

	void parseBody();

	void cleanBuffer();

	// state
	bool	complete_;
	bool	requestLineParsed_;
	bool	headerParsed_;
	int		errorCode_;
	size_t	maxBodySize_;
	bool	maxBodySizeSet_;
	size_t	bytesParsed_;
	size_t	contentLength_;
	bool	chunked_;
	bool	connectionClose_;

	// parsed data
	std::string							rawBuffer_;
	std::string							method_;
	std::string							uri_;
	std::string							version_;
	std::map<std::string, std::string>	headers_;
	std::string							host_;
	std::string							body_;

};
