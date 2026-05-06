#include "http/HttpRequest.hpp"		// MAX_HEADER_SIZE
#include "http/HttpStatusCodes.hpp"	// BadRequest, RequestHeaderFieldsTooLarge
#include "utils/StringUtils.hpp"	// toLower, trim, hasInvalidChar
#include "Defaults.hpp"				// MAX_HEADER_SIZE

#include <cstdlib>					// size_t, strtoul
#include <string>					// string

using namespace HttpStatus;
using namespace StringUtils;

static bool isValidHeaderFieldName(const std::string &name)
{
	static const std::string allowedSpecials("!#$%&'*+-.^_`|~");
	if (name.empty())
		return false;
	for (size_t i = 0; i < name.size(); i++)
	{
		unsigned char c = name.at(i);
		if (!std::isalnum(c) && allowedSpecials.find(c) == std::string::npos)
			return false;
	}
	return true;
}

static bool isValidHeaderFieldValue(const std::string &value)
{
	for (size_t i = 0; i < value.size(); ++i)
	{
		unsigned char c = value.at(i);
		if (c == 0x7f || (c <= 0x1f && c != '\t'))
			return false;
	}
	return true;
}

static bool isChunkedEncoding(const std::string &transferEncoding)
{
	std::string last;
	size_t pos = 0;
	while (true)
	{
		size_t comma = transferEncoding.find(',', pos);
		std::string token = trim(transferEncoding.substr(pos, comma == std::string::npos ? std::string::npos : comma - pos));
		if (!token.empty())
			last = token;
		if (comma == std::string::npos)
			break;
		pos = comma + 1;
	}
	return toLower(last) == "chunked";
}

static int validateHeaderFieldsFormat(const std::string &rawHeaderFields)
{
	for (size_t pos = 0; (pos = rawHeaderFields.find('\n', pos)) != std::string::npos; ++pos)
		if (pos == 0 || rawHeaderFields.at(pos - 1) != '\r')
			return BadRequest;

	if (rawHeaderFields.size() > MAX_HEADER_SIZE)
		return RequestHeaderFieldsTooLarge;

	return VALID;
}

static int parseHeaderField(const std::string &line, bool &seenContentLength, size_t &contentLength, bool &chunked, std::string &host, bool &connectionClose, std::map<std::string, std::string> &parsed)
{
	size_t colonPos = line.find(':');
	if (colonPos == std::string::npos)
		return BadRequest;

	std::string name	= toLower(line.substr(0, colonPos));
	std::string value	= trim(line.substr(colonPos + 1));

	if (!isValidHeaderFieldName(name) || !isValidHeaderFieldValue(value))
		return BadRequest;

	if (name == "content-length")
	{
		if (seenContentLength)
			return BadRequest;
		seenContentLength = true;
		if (value.empty() || !std::isdigit((unsigned char)value.at(0)))
			return BadRequest;
		char *end;
		contentLength = std::strtoul(value.c_str(), &end, 10);
		if (*end)
			return BadRequest;
	}
	else if (name == "transfer-encoding")
		chunked = isChunkedEncoding(value);
	else if (name == "connection")
	{
		size_t pos = 0;
		while (pos < value.size())
		{
			size_t comma = value.find(',', pos);
			std::string token = toLower(trim(value.substr(pos, comma == std::string::npos ? std::string::npos : comma - pos)));
			if (token == "close")
			{
				connectionClose = true;
				break ;
			}
			else if (token == "keep-alive")
				connectionClose = false;
			if (comma == std::string::npos)
				break ;
			pos = comma + 1;
		}
	}
	else if (name == "host")
	{
		static const std::string allowedSpecials = ".-_~!$&'()*+,;=%";
		size_t		colon	= value.find(':');
		std::string	hostTmp	= value.substr(0, colon);
		for (size_t i = 0; i < hostTmp.size(); ++i)
			if (!std::isalnum((unsigned char)hostTmp[i]) && allowedSpecials.find(hostTmp[i]) == std::string::npos)
				return BadRequest;
		if (colon != std::string::npos && !StringUtils::isAllDigits(value, colon + 1))
			return BadRequest;
		host = hostTmp;
	}

	parsed[name] = value;
	return VALID;
}

static int resolveBodyEncoding(const std::string &version, bool chunked, const std::map<std::string, std::string> &parsed)
{
	if (chunked && version == "HTTP/1.0")
		return BadRequest;
	if (version == "HTTP/1.1" && parsed.find("host") == parsed.end())
		return BadRequest;
	return VALID;
}

int validateHeaders(const std::string &rawHeaderFields, bool complete, const std::string &version, size_t &contentLength, bool &chunked, std::string &host, bool &connectionClose, std::map<std::string, std::string> &parsed)
{
	if (int result = validateHeaderFieldsFormat(rawHeaderFields))
		return result;

	if (!complete)
		return NEED_MORE_DATA;

	bool	seenContentLength = false;
	size_t	pos = 0;
	while (pos < rawHeaderFields.size())
	{
		size_t lineEnd = rawHeaderFields.find("\r\n", pos);
		std::string line = (lineEnd == std::string::npos)
			? rawHeaderFields.substr(pos)
			: rawHeaderFields.substr(pos, lineEnd - pos);

		if (int result = parseHeaderField(line, seenContentLength, contentLength, chunked, host, connectionClose, parsed))
			return result;

		if (lineEnd == std::string::npos)
			break;
		pos = lineEnd + 2;
	}

	return resolveBodyEncoding(version, chunked, parsed);
}
