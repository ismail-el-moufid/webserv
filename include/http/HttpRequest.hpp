#pragma once

#include "config/VirtualHost.hpp"	// VirtualHost
#include "http/HttpStatusCodes.hpp"	// Code
#include "http/HttpRequestBody.hpp"	// HttpRequestBody

#include <string>					// string
#include <map>						// map

#define VALID 0
#define NEED_MORE_DATA 0

class HttpRequest
{

public:

	class URI
	{

		public:

			std::string	uri;
			std::string	path;
			std::string	query;
			bool		hasQuery;

			void clear();
	};

	HttpRequest();

	void parse(const std::string &rawBytes);

	// post-header parsing methods
	void	setMaxBodySize(size_t maxBodySize);
	void	initBody(int fd);
	void	initBodyRaw(const std::string &uploadDir);
	void	initBodyRaw(const std::string &uploadDir, const std::string &filename);
	void	initBodyMultipart(const std::string &uploadDir, const std::string &boundary);
	int		drainBody();

	void reset();

	bool										complete() const;
	bool										headersComplete() const;
	const std::string							&method() const;
	const URI									&uri() const;
	const std::string							&version() const;
	const std::map<std::string, std::string>	&headers() const;
	const std::string							&host() const;
	bool										expectsContinue() const;
	const std::string							&contentType() const;
	size_t										contentLength() const;
	bool										hasCgi() const;
	bool										erroneous() const;
	bool										connectionClose() const;
	HttpStatus::Code							errorCode() const;

	// config context
	const VirtualHost	*vhost;
	const Route			*route;

private:

	HttpRequest(const HttpRequest &);
	HttpRequest &operator=(const HttpRequest &);

	bool parseRequestLine();

	bool parseHeaders();

	void parseBody();

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
	bool	expectsContinue_;

	// parsed data
	std::string							rawBuffer_;
	std::string							method_;
	URI									uri_;
	std::string							version_;
	std::map<std::string, std::string>	headers_;
	std::string							host_;
	std::string							contentType_;
	HttpRequestBody						body_;

};
