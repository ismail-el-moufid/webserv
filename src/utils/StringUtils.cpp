#include "utils/StringUtils.hpp"	// toLower, trim, isAllDigits, hasInvalidChar
#include <cctype>					// isdigit, isspace, tolower, toupper
#include <ctime>					// localtime, strftime, time_t

namespace StringUtils
{

std::string toLower(const std::string &s)
{
	std::string result = s;
	for (size_t i = 0; i < result.size(); ++i)
		result[i] = std::tolower((unsigned char)result[i]);
	return result;
}

std::string trim(const std::string &s)
{
	size_t start = 0;
	while (start < s.size() && std::isspace((unsigned char)s.at(start)))
		++start;
	size_t end = s.size();
	while (end > start && std::isspace((unsigned char)s.at(end - 1)))
		--end;
	return s.substr(start, end - start);
}

std::string normalizeSlashes(const std::string &target)
{
	std::string result;
	for (size_t i = 0; i < target.size(); ++i)
		if (target[i] != '/' || result.empty() || result.back() != '/')
			result += target[i];
	return result;
}

bool isAllDigits(const std::string &s, size_t start, size_t end)
{
	size_t realEnd = (end == std::string::npos) ? s.size() : end;
	if (start >= realEnd)
		return false;
	for (size_t i = start; i < realEnd; i++)
		if (!std::isdigit( static_cast<unsigned char>(s.at(i))))
			return false;
	return true;
}

std::vector<const char *> toNullTerminatedCStrings(const std::vector<std::string> &stringVector)
{
	std::vector<const char *> result;
	result.reserve(stringVector.size() + 1);
	for (size_t i = 0; i < stringVector.size(); ++i)
		result.push_back(stringVector[i].c_str());
	result.push_back(NULL);
	return result;
}

bool hasInvalidChar(const std::string &s, const std::string &invalid, size_t from) { return s.find_first_of(invalid, from) != std::string::npos; }

std::string currentTime()
{
	std::time_t	now = std::time(NULL);
	char		buf[32];

	std::strftime(buf, sizeof(buf), "[%d/%b/%Y:%H:%M:%S %z]", std::localtime(&now));

	return buf;
}

}
