#include "http/HttpRequestBody.hpp"
#include "http/HttpStatusCodes.hpp"

#include <unistd.h>
#include <ctime>
#include <sstream>
#include <sys/stat.h>

HttpRequestBody::HttpRequestBody() : errorCodePtr_(NULL), errorOccurred_(false), file_(NULL), uploadState_(INACTIVE), size_(0), fd_(-1), offset_(0) { }

HttpRequestBody::~HttpRequestBody() { delete file_; }

void HttpRequestBody::init(int fd) { fd_ = fd; }

void HttpRequestBody::initRaw(const std::string &uploadDir, int &errorCode)
{
	errorCodePtr_	= &errorCode;
	uploadDir_		= uploadDir;

	std::ostringstream path;
	path << uploadDir << "/upload_" << std::time(NULL);
	file_ = new std::ofstream(path.str().c_str(), std::ios::binary);
	if (!file_->is_open())
		{
			errorOccurred_	= true;
			*errorCodePtr_	= HttpStatus::InternalServerError;
			delete file_;
			file_ = NULL;
		}

	uploadState_	= STREAMING;
}

void HttpRequestBody::initRaw(const std::string &uploadDir, const std::string &filename, int &errorCode)
{
	errorCodePtr_	= &errorCode;
	uploadDir_		= uploadDir;

	// Use the provided filename (from URL path) if non-empty, otherwise timestamp fallback.
	std::string name = filename;
	if (name.empty())
	{
		std::ostringstream ts;
		ts << "upload_" << std::time(NULL);
		name = ts.str();
	}

	std::string path = uploadDir + "/" + name;
	file_ = new std::ofstream(path.c_str(), std::ios::binary);
	if (!file_->is_open())
	{
		errorOccurred_	= true;
		*errorCodePtr_	= HttpStatus::InternalServerError;
		delete file_;
		file_ = NULL;
	}

	uploadState_	= STREAMING;
}

void HttpRequestBody::initMultipart(const std::string &uploadDir, const std::string &boundary, int &errorCode)
{
	errorCodePtr_	= &errorCode;
	uploadDir_		= uploadDir;
	boundary_		= boundary;
	headerBuf_		= "--" + boundary + "\r\n";

	uploadState_	= WAITING_PART_HEADERS;
}

// Extract filename from part headers, sanitize, return empty on failure
static std::string extractFilename(const std::string &headers)
{
	size_t fn = headers.find("filename=\"");
	if (fn == std::string::npos)
		return "";
	fn += 10;
	size_t fnEnd = headers.find('"', fn);
	if (fnEnd == std::string::npos)
		return "";
	std::string name = headers.substr(fn, fnEnd - fn);
	size_t slash = name.find_last_of("/\\");
	if (slash != std::string::npos)
		name = name.substr(slash + 1);
	return name;
}

void HttpRequestBody::openFile(const std::string &partHeaders)
{
	delete file_;
	file_ = NULL;

	std::string base = extractFilename(partHeaders);
	std::string name;
	if (!base.empty())
		name = "upload_" + base;
	else
		name = "upload_" + std::to_string(std::time(NULL));

	std::string path = uploadDir_ + "/" + name;
	file_ = new std::ofstream(path.c_str(), std::ios::binary);
	if (!file_->is_open())
	{
		errorOccurred_	= true;
		*errorCodePtr_	= HttpStatus::InternalServerError;
		delete file_;
		file_ = NULL;
	}
}

void HttpRequestBody::processMultipart(const std::string &chunk)
{
	if (errorOccurred_)
		return ;

	headerBuf_ += chunk;

	const std::string delim = "\r\n--" + boundary_;
	while (uploadState_ != DONE)
	{
		if (uploadState_ == WAITING_PART_HEADERS)
		{
			// find end of part headers
			size_t hEnd = headerBuf_.find("\r\n\r\n");
			if (hEnd == std::string::npos)
				return ; // need more data

			std::string partHeaders	= headerBuf_.substr(0, hEnd);
			std::string remaining	= headerBuf_.substr(hEnd + 4);

			openFile(partHeaders);
			if (errorOccurred_)
				return ;

			headerBuf_.clear();
			uploadState_ = STREAMING;

			// remaining is data after the headers — fall through
			headerBuf_ = remaining;
		}

		if (uploadState_ == STREAMING)
		{
			// check if delimiter is present in buffer
			size_t delimPos	= headerBuf_.find(delim);
			if (delimPos != std::string::npos)
			{
				// write everything before the delimiter
				if (file_ && delimPos > 0)
				{
					file_->write(headerBuf_.c_str(), delimPos);
					if (file_->fail())
					{
						errorOccurred_ = true;
						*errorCodePtr_ = HttpStatus::InternalServerError;
						return ;
					}
				}

				std::string after = headerBuf_.substr(delimPos + delim.size());

				// is it the closing boundary (--) or a new part (\r\n)?
				if (after.size() >= 2 && after[0] == '-' && after[1] == '-')
				{
					uploadState_ = DONE;
					headerBuf_.clear();
					return ;
				}
				else if (after.size() >= 2 && after[0] == '\r' && after[1] == '\n')
				{
					// next part — prime header buffer
					headerBuf_		= after.substr(2);
					uploadState_	= WAITING_PART_HEADERS;
					continue ; // process next part
				}
				else
				{
					// delimiter found but incomplete suffix — wait for more data
					// keep everything from delimPos onward
					headerBuf_ = headerBuf_.substr(delimPos);
					return ;
				}
			}
			else
			{
				// no delimiter yet — safe to flush all but the last (delim.size()-1) bytes
				// to avoid splitting the delimiter across chunks
				size_t safeEnd = headerBuf_.size() > delim.size()
					? headerBuf_.size() - delim.size()
					: 0;
				if (file_ && safeEnd > 0)
				{
					file_->write(headerBuf_.c_str(), safeEnd);
					if (file_->fail())
					{
						errorOccurred_ = true;
						*errorCodePtr_ = HttpStatus::InternalServerError;
						return ;
					}
				}
				headerBuf_ = headerBuf_.substr(safeEnd);
				return ;
			}
		}
	}
}

HttpRequestBody &HttpRequestBody::operator+=(const std::string &chunk)
{
	size_ += chunk.size();

	if (uploadState_ != INACTIVE)
	{
		if (!boundary_.empty())
			processMultipart(chunk);
		else if (file_) // raw upload
		{
			file_->write(chunk.c_str(), chunk.size());
			if (file_->fail())
			{
				errorOccurred_ = true;
				*errorCodePtr_ = HttpStatus::InternalServerError;
				return *this;
			}
		}
		return *this;
	}

	if (fd_ != -1)
		data_ += chunk;

	// intentionally discard normal requests — don't need body in memory
	return *this;
}

void HttpRequestBody::reset()
{
	data_.clear();
	headerBuf_.clear();
	boundary_.clear();
	uploadDir_.clear();

	errorOccurred_	= false;
	errorCodePtr_	= NULL;

	size_			= 0;
	fd_				= -1;
	offset_			= 0;

	uploadState_	= INACTIVE;
	delete file_;
	file_ = NULL;
}

size_t	HttpRequestBody::size()		const { return size_; }
bool	HttpRequestBody::empty()	const { return data_.empty(); }

int HttpRequestBody::drain()
{
	if (file_ || fd_ == -1)
		return 1;

	if (data_.empty())
		return 1;

	ssize_t written = write(fd_, data_.c_str() + offset_, data_.size() - offset_);
	if (written > 0)
	{
		offset_ += written;
		if (offset_ >= data_.size())
		{
			data_.clear();
			offset_ = 0;
			return 1;
		}
	}
	else if (written == -1)
		return -1;

	return 0;
}
