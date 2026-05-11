#include "http/HttpRequestBody.hpp"

#include <unistd.h>
#include <ctime>
#include <sstream>
#include <sys/stat.h>

HttpRequestBody::HttpRequestBody() : file_(NULL), uploadState_(INACTIVE), size_(0), fd_(-1) {}

HttpRequestBody::~HttpRequestBody() { delete file_; }

void HttpRequestBody::init(int fd) { fd_ = fd; }

void HttpRequestBody::init(const std::string &uploadDir, const std::string &boundary)
{
	uploadDir_		= uploadDir;
	boundary_		= boundary;
	uploadState_	= WAITING_PART_HEADERS;

	// prime headerBuf_ with the opening boundary so part header search works uniformly
	headerBuf_		= "--" + boundary + "\r\n";
}

void HttpRequestBody::init(const std::string &uploadDir)
{
	uploadDir_		= uploadDir;
	uploadState_	= STREAMING;

	// open a timestamped file immediately for raw uploads
	std::ostringstream path;
	path << uploadDir << "/upload_" << std::time(NULL);
	file_ = new std::ofstream(path.str().c_str(), std::ios::binary);
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

	std::string name = extractFilename(partHeaders);
	if (name.empty())
	{
		std::ostringstream ts;
		ts << "upload_" << std::time(NULL);
		name = ts.str();
	}
	std::string path	= uploadDir_ + "/" + name;
	file_				= new std::ofstream(path.c_str(), std::ios::binary);
}

void HttpRequestBody::processMultipart(const std::string &chunk)
{
	headerBuf_ += chunk;

	while (uploadState_ != DONE)
	{
		if (uploadState_ == WAITING_PART_HEADERS)
		{
			// find end of part headers
			size_t hEnd = headerBuf_.find("\r\n\r\n");
			if (hEnd == std::string::npos)
				return; // need more data

			std::string partHeaders	= headerBuf_.substr(0, hEnd);
			std::string remaining	= headerBuf_.substr(hEnd + 4);

			openFile(partHeaders);
			headerBuf_.clear();
			uploadState_ = STREAMING;

			// remaining is data after the headers — fall through
			headerBuf_ = remaining;
		}

		if (uploadState_ == STREAMING)
		{
			std::string delim	= "\r\n--" + boundary_;

			// check if delimiter is present in buffer
			size_t delimPos		= headerBuf_.find(delim);
			if (delimPos != std::string::npos)
			{
				// write everything before the delimiter
				if (file_ && delimPos > 0)
					file_->write(headerBuf_.c_str(), delimPos);

				std::string after = headerBuf_.substr(delimPos + delim.size());

				// is it the closing boundary (--) or a new part (\r\n)?
				if (after.size() >= 2 && after[0] == '-' && after[1] == '-')
				{
					uploadState_ = DONE;
					headerBuf_.clear();
					return;
				}
				else if (after.size() >= 2 && after[0] == '\r' && after[1] == '\n')
				{
					// next part — prime header buffer
					headerBuf_		= after.substr(2);
					uploadState_	= WAITING_PART_HEADERS;
					continue; // process next part
				}
				else
				{
					// delimiter found but incomplete suffix — wait for more data
					// keep everything from delimPos onward
					headerBuf_ = headerBuf_.substr(delimPos);
					return;
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
					file_->write(headerBuf_.c_str(), safeEnd);
				headerBuf_ = headerBuf_.substr(safeEnd);
				return;
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
			file_->write(chunk.c_str(), chunk.size());
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

	size_			= 0;
	fd_				= -1;

	uploadState_	= INACTIVE;
	delete file_;
	file_ = NULL;
}

size_t	HttpRequestBody::size()		const { return size_; }
bool	HttpRequestBody::empty()	const { return data_.empty(); }
