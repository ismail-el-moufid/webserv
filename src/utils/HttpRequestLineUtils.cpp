#include "http/HttpRequest.hpp"		// NEED_MORE_DATA, VALID
#include "http/HttpStatusCodes.hpp"	// BadRequest, URITooLong, HTTPVersionNotSupported, NotImplemented
#include "utils/StringUtils.hpp"	// isAllDigits
#include "Defaults.hpp"				// MAX_REQUEST_LINE_SIZE

#include <string>					// string

using namespace HttpStatus;
using namespace StringUtils;

// Method

static bool isValidMethodChar(unsigned char c)
{
	static const std::string allowedSpecials("!#$%&'*+-.^_`|~");
	return std::isalnum(c) || allowedSpecials.find(c) != std::string::npos;
}

static bool isValidMethod(const std::string &method)
{
	for (size_t i = 0; i < method.size(); i++)
		if (!isValidMethodChar(method.at(i)))
			return false;
	return true;
}

int validateMethod(const std::string &method)
{
	if (method.empty() || !isValidMethod(method))
		return BadRequest;
	return VALID;
}

// URI

static int validateURI(size_t firstSpace, size_t lastSpace)
{
	if (lastSpace <= firstSpace + 1)
		return BadRequest;
	if (lastSpace - firstSpace - 1 > MAX_URI_SIZE)
		return URITooLong;
	return VALID;
}

// Version

static bool isKnownVersion(const std::string &version)
{
	return version == "HTTP/1.0" || version == "HTTP/1.1";
}

static bool hasFullHttpPrefix(const std::string &version)
{
	return version.size() >= 5 && version.compare(0, 5, "HTTP/") == 0;
}

static bool isIncompleteHttpPrefix(const std::string &version)
{
	return version.size() <= 5 && std::string("HTTP/").find(version) == 0;
}

static int validateVersionNumbers(const std::string &version)
{
	size_t dot = version.find('.', 5);
	if (dot == std::string::npos || !isAllDigits(version, 5, dot) || !isAllDigits(version, dot + 1, version.size()))
		return BadRequest;
	return HTTPVersionNotSupported;
}

int validateVersion(const std::string &version, bool endFound)
{
	if (isKnownVersion(version))
		return VALID;

	if (!hasFullHttpPrefix(version))
	{
		if (isIncompleteHttpPrefix(version))
			return endFound ? BadRequest : NEED_MORE_DATA;
		return BadRequest;
	}

	if (!endFound && version.size() < 16)
		return NEED_MORE_DATA;

	return validateVersionNumbers(version);
}

// Request line

static bool locateSpaces(const std::string &line, size_t &firstSpace, size_t &lastSpace, size_t &spaceCount)
{
	spaceCount	= 0;
	firstSpace	= std::string::npos;
	lastSpace	= std::string::npos;

	for (size_t i = 0; i < line.size(); ++i)
	{
		if (line[i] == ' ')
		{
			if (firstSpace == std::string::npos)
				firstSpace = i;
			lastSpace = i;
			++spaceCount;
		}
	}
	return spaceCount != 0;
}

int validateRequestLine(const std::string &line, size_t &firstSpace, size_t &lastSpace, bool endFound)
{
	// only move on if line is fully received or overly long
	if (!endFound && line.size() <= MAX_REQUEST_LINE_SIZE)
			return NEED_MORE_DATA;

	if (hasInvalidChar(line, std::string("\n\0", 2)))
		return BadRequest;
	if (endFound && hasInvalidChar(line, "\r"))
		return BadRequest;

	size_t spaceCount;
	if (!locateSpaces(line, firstSpace, lastSpace, spaceCount))
		return endFound ? BadRequest : NEED_MORE_DATA;
	if (spaceCount > 2)
		return BadRequest;

	if (int result = validateMethod(line.substr(0, firstSpace)))
		return result;

	if (int result = validateURI(firstSpace, lastSpace))
		return result;

	if (spaceCount == 1)
		return endFound ? BadRequest : NEED_MORE_DATA;

	return validateVersion(line.substr(lastSpace + 1), endFound);
}
