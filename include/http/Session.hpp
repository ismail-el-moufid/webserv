#pragma once

#include <string>
#include <map>

namespace Session
{
	std::string							generate();
	std::string							parseSid(const std::string &cookieHeader);
	std::map<std::string, std::string>	load(const std::string &sid);
	void								save(const std::string &sid, const std::map<std::string, std::string> &data);
	void								destroy(const std::string &sid);
}
