#pragma once

#include "config/VirtualHost.hpp"






#include <string>					// string
#include <map>						// map






#define MAX_REQUEST_LINE_SIZE	8192	// 8KB
#define MAX_HEADER_SIZE			16384	// 16KB
#define MAX_URI_SIZE			(MAX_REQUEST_LINE_SIZE - 15)
#define VALID					0
#define NEED_MORE_DATA			0





class HttpRequest
{

public:

	HttpRequest(size_t maxBodySize);

	void parse(const std::string &rawBytes);

	void reset();

	bool										complete() const;
	const std::string							&method() const;
	const std::string							&uri() const;
	const std::string							&version() const;
	const std::map<std::string, std::string>	&headers() const;
	const std::string							&body() const;
	size_t										contentLength() const;
	int											errorCode() const;

	// config context
	const VirtualHost	*vhost;
	const Route			*route;

private:

	// can't be default constructed, must provide max body size
	HttpRequest();

	bool parseRequestLine();

	bool parseHeaders();

	void parseBody();

	void cleanBuffer();

	// state
	bool	complete_;
	bool	headerParsed_;
	bool	requestLineParsed_;
	int		errorCode_;
	size_t	maxBodySize_;
	size_t	bytesParsed_;
	size_t	contentLength_;
	bool	chunked_;

	// parsed data
	std::string							rawBuffer_;
	std::string							method_;
	std::string							uri_;
	std::string							version_;
	std::map<std::string, std::string>	headers_;
	std::string							body_;

};
