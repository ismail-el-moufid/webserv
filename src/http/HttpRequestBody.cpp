#include "http/HttpStatusCodes.hpp"	// BadRequest, ContentTooLarge

#include <string>					// string
#include <cstdlib>					// strtoul
#include <algorithm>				// min

using namespace HttpStatus;

static bool parseChunkSize(std::string &rawBuffer, size_t &bytesParsed, std::string &body, int &errorCode, size_t &chunkSize)
{
	size_t lineEnd = rawBuffer.find("\r\n", bytesParsed);
	if (lineEnd == std::string::npos)
		return false;

	std::string sizeStr = rawBuffer.substr(bytesParsed, lineEnd - bytesParsed);

	size_t extPos = sizeStr.find(';');
	if (extPos != std::string::npos)
		sizeStr = sizeStr.substr(0, extPos);

	if (sizeStr.empty() || sizeStr.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos)
		return (errorCode = BadRequest, body.clear(), false);

	char *end;
	chunkSize = std::strtoul(sizeStr.c_str(), &end, 16);
	if (*end != '\0')
		return (errorCode = BadRequest, body.clear(), false);

	bytesParsed = lineEnd + 2;
	return true;
}

static bool appendChunk(std::string &rawBuffer, size_t &bytesParsed, std::string &body, int &errorCode, size_t maxBodySize, size_t chunkSize)
{
	size_t chunkEnd = bytesParsed + chunkSize;

	if (rawBuffer.size() < chunkEnd + 2)
		return false;

	if (body.size() + chunkSize > maxBodySize)
		return (errorCode = ContentTooLarge, body.clear(), false);

	body += rawBuffer.substr(bytesParsed, chunkSize);
	bytesParsed = chunkEnd + 2;
	return true;
}

bool parseChunkedBody(std::string &rawBuffer, size_t &bytesParsed, std::string &body, bool &complete, int &errorCode, size_t maxBodySize)
{
	size_t localParsed = bytesParsed;
	size_t chunkSize;

	if (!parseChunkSize(rawBuffer, localParsed, body, errorCode, chunkSize))
		return false;

	if (chunkSize == 0)
		return bytesParsed = localParsed, complete = true, true;

	if (!appendChunk(rawBuffer, localParsed, body, errorCode, maxBodySize, chunkSize))
		return false;

	bytesParsed = localParsed;
	return true;
}

bool parseFixedBody(std::string &rawBuffer, size_t &bytesParsed, std::string &body, bool &complete, size_t contentLength)
{
	size_t bytesAvailable	= rawBuffer.size() - bytesParsed;
	size_t bytesToParse		= std::min(bytesAvailable, contentLength - body.size());

	if (bytesToParse == 0)
		return false;

	body += rawBuffer.substr(bytesParsed, bytesToParse);
	bytesParsed += bytesToParse;

	if (body.size() == contentLength)
		complete = true;

	return true;
}
