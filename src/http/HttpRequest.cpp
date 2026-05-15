#include "http/HttpRequest.hpp"		// HttpRequest
#include "http/HttpRequestBody.hpp"	// HttpRequestBody
#include "http/HttpStatusCodes.hpp"	// BadRequest, ContentTooLarge
#include "utils/StringUtils.hpp"	// normalizeSlashes

#include <string>					// string
#include <map>						// map
#include <cstdlib>					// strtoul

using namespace HttpStatus;

int validateRequestLine(const std::string &line, size_t &firstSpace, size_t &lastSpace, bool complete);

bool HttpRequest::parseRequestLine()
{
	if (requestLineParsed_)
		return true;

	std::string line;

	size_t firstSpace	= 0, lastSpace = 0;

	size_t lineEnd		= rawBuffer_.find("\r\n");
	if (lineEnd == std::string::npos)
		return errorCode_ = validateRequestLine(rawBuffer_, firstSpace, lastSpace, false), false;

	line = rawBuffer_.substr(0, lineEnd);

	if ((errorCode_ = validateRequestLine(line, firstSpace, lastSpace, true)))
		return false;

	method_				= line.substr(0, firstSpace);

	uri_.uri			= line.substr(firstSpace + 1, lastSpace - firstSpace - 1);
	size_t queryDel		= uri_.uri.find('?');

	uri_.hasQuery		= queryDel != std::string::npos;
	uri_.path			= StringUtils::normalizeSlashes(uri_.uri.substr(0, queryDel));
	uri_.query			= (uri_.hasQuery ? uri_.uri.substr(queryDel + 1) : "");

	version_			= line.substr(lastSpace + 1);
	connectionClose_	= (version_ != "HTTP/1.1");

	bytesParsed_		= lineEnd + 2;
	requestLineParsed_	= true;

	return true;
}

int validateHeaders(const std::string &rawHeaderFields, bool complete, const std::string &version, size_t &contentLength, bool &chunked, std::string &host, bool &connectionClose, bool &expectsContinue, std::string &contentType, std::map<std::string, std::string> &parsed);

bool HttpRequest::parseHeaders()
{
	if (headerParsed_)
		return true;

	size_t	headersEnd	= rawBuffer_.find("\r\n\r\n", bytesParsed_);
	bool	complete	= (headersEnd != std::string::npos);

	size_t advance = 4;
	if (!complete && rawBuffer_.size() >= bytesParsed_ + 2 && rawBuffer_.compare(bytesParsed_, 2, "\r\n") == 0)
	{
		headersEnd	= bytesParsed_;
		complete	= true;
		advance		= 2;
	}

	std::string rawHeaderFields = complete ? rawBuffer_.substr(bytesParsed_, headersEnd - bytesParsed_)
								: rawBuffer_.substr(bytesParsed_);

	if ((errorCode_ = validateHeaders(rawHeaderFields, complete, version_, contentLength_, chunked_, host_, connectionClose_, expectsContinue_, contentType_, headers_))
		|| !complete)
		return false;

	if (!chunked_ && maxBodySizeSet_ && contentLength_ > maxBodySize_)
		return errorCode_ = ContentTooLarge, false;

	if (!chunked_ && contentLength_ == 0)
		complete_ = true;

	bytesParsed_	= headersEnd + advance;
	headerParsed_	= true;
	return true;
}

void parseFixedBody(std::string &rawBuffer, size_t &bytesParsed, HttpRequestBody &body, bool &complete, size_t contentLength);
void parseChunkedBody(std::string &rawBuffer, size_t &bytesParsed, HttpRequestBody &body, bool &complete, int &errorCode, size_t maxBodySize, bool maxBodySizeSet);

void HttpRequest::parseBody()
{
	if (chunked_)
	{
		while (!complete_)
			parseChunkedBody(rawBuffer_, bytesParsed_, body_, complete_, errorCode_, maxBodySize_, maxBodySizeSet_);
		
		// Normalize headers to reflect the decoded request, not the wire format
		contentLength_ = body_.size();
		headers_.erase("transfer-encoding");
		headers_["content-length"] = StringUtils::toString(contentLength_);
	}
	else
		parseFixedBody(rawBuffer_, bytesParsed_, body_, complete_, contentLength_);
}

void HttpRequest::parse(const std::string &rawBytes)
{
	if (complete_ || errorCode_)
		return ;

	rawBuffer_ += rawBytes;
	if (!headerParsed_)
	{
		if (!parseRequestLine())
			return ;
		if (!parseHeaders())
			return ;
		return ; // pause here caller must set maxBodySize then call parse("") to continue
	}
	parseBody();
}

void	HttpRequest::initBody(int fd)																{ body_.init(fd); }
void	HttpRequest::initBodyRaw(const std::string &uploadDir)										{ body_.initRaw(uploadDir, errorCode_); }
void	HttpRequest::initBodyMultipart(const std::string &uploadDir, const std::string &boundary)	{ body_.initMultipart(uploadDir, boundary, errorCode_); }
void	HttpRequest::initBodyRaw(const std::string &uploadDir, const std::string &filename)			{ body_.initRaw(uploadDir, filename, errorCode_); }
int		HttpRequest::drainBody()																	{ return body_.drain(); }

void HttpRequest::URI::clear()
{
	uri.clear();
	path.clear();
	query.clear();
	hasQuery = false;
}

void HttpRequest::reset()
{
	vhost				= NULL;
	route				= NULL;
	complete_			= false;
	requestLineParsed_	= false;
	headerParsed_		= false;
	chunked_			= false;
	maxBodySizeSet_		= false;
	connectionClose_	= true;
	errorCode_			= 0;
	contentLength_		= 0;
	maxBodySize_		= 0;
	method_.clear();
	uri_.clear();
	version_.clear();
	headers_.clear();
	host_.clear();
	body_.reset();
	if (bytesParsed_ > 0)
	{
		if (bytesParsed_ < rawBuffer_.size())
			rawBuffer_.erase(0, bytesParsed_);
		else
			rawBuffer_.clear();
	}
	bytesParsed_ = 0;
	parse(""); // in case there is already data in rawBuffer_ (pipelined request)
}

bool HttpRequest::hasCgi() const
{
	if (!route || route->cgis().empty())
		return false;

	std::string path = uri_.path;

	for (std::map<std::string, std::string>::const_iterator it = route->cgis().begin(); it != route->cgis().end(); ++it)
	{
		const std::string&	ext		= it->first;
		size_t				ext_pos	= path.find(ext);
		if (ext_pos != std::string::npos && (ext_pos + ext.length() == path.length() || path[ext_pos + ext.length()] == '/'))
			return true;
	}

	return false;
}

HttpRequest::HttpRequest() : vhost(NULL), route(NULL), complete_(false), requestLineParsed_(false), headerParsed_(false), errorCode_(0), maxBodySizeSet_(false), bytesParsed_(0), contentLength_(0), chunked_(false), connectionClose_(true), expectsContinue_(false) { }

void HttpRequest::setMaxBodySize(size_t maxBodySize) { maxBodySize_ = maxBodySize; maxBodySizeSet_ = true; }

bool										HttpRequest::complete()			const { return complete_ || errorCode_ != 0; }
bool										HttpRequest::headersComplete()	const { return headerParsed_; }
const std::string							&HttpRequest::method()			const { return method_; }
const HttpRequest::URI						&HttpRequest::uri()				const { return uri_; }
const std::string							&HttpRequest::version()			const { return version_; }
const std::map<std::string, std::string>	&HttpRequest::headers()			const { return headers_; }
size_t										HttpRequest::contentLength()	const { return contentLength_; }
const std::string							&HttpRequest::host()			const { return host_; }
bool										HttpRequest::erroneous()		const { return errorCode_ != 0; }
bool										HttpRequest::connectionClose()	const { return connectionClose_; }
HttpStatus::Code							HttpRequest::errorCode()		const { return static_cast<Code>(errorCode_); }
bool										HttpRequest::expectsContinue()	const { return expectsContinue_; }
const std::string							&HttpRequest::contentType()		const { return contentType_; }
