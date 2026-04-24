#include "utils/StringUtils.hpp"	// toLower, trim, isAllDigits, hasInvalidChar

namespace StringUtils
{

std::string toLower(const std::string &s)
{
	std::string result = s;
	for (size_t i = 0; i < result.size(); ++i)
		result.at(i) = std::tolower((unsigned char)result.at(i));
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
		if (target.at(i) != '/' || result.empty() || result.back() != '/')
			result += target.at(i);
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

bool hasInvalidChar(const std::string &s, const std::string &invalid, size_t from) { return s.find_first_of(invalid, from) != std::string::npos; }

}
