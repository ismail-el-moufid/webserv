#pragma once








#include <string>					// string
#include <sstream>					// ostringstream
#include <vector>					// vector















namespace StringUtils
{

	std::string					toLower(const std::string &s);
	std::string					toUpper(const std::string &s);
	std::string					trim(const std::string &s);
	std::string					normalizeSlashes(const std::string &target);
	bool						isAllDigits(const std::string &s, size_t start = 0, size_t end = std::string::npos);
	bool						hasInvalidChar(const std::string &s, const std::string &invalidChars, size_t from = 0);
	std::vector<const char *>	toNullTerminatedCStrings(const std::vector<std::string> &stringVector);

	template <typename T>
	std::string toString(const T &value)
	{
		std::ostringstream oss;
		oss << value;
		return oss.str();
	}

}
