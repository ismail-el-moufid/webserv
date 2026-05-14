#include "http/Session.hpp"
#include "Defaults.hpp"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cctype>
#include <cstdlib>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>



namespace Session
{

static void ensureDir()
{
	struct stat st;
	if (stat(SESSION_DIR, &st) != 0)
		mkdir(SESSION_DIR, 0700);
}

static bool validSid(const std::string &sid)
{
	if (sid.empty() || sid.size() > 64)
		return false;
	for (size_t i = 0; i < sid.size(); ++i)
		if (!std::isxdigit(static_cast<unsigned char>(sid[i])))
			return false;
	return true;
}

std::string generate()
{
	ensureDir();
	unsigned char buf[16];
	int fd = open("/dev/urandom", O_RDONLY);
	if (fd != -1)
	{
		read(fd, buf, sizeof(buf));
		close(fd);
	}
	else
	{
		for (int i = 0; i < 16; ++i)
			buf[i] = static_cast<unsigned char>(rand() ^ (static_cast<int>(std::time(NULL)) >> i));
	}
	std::ostringstream ss;
	ss << std::hex << std::setfill('0');
	for (int i = 0; i < 16; ++i)
		ss << std::setw(2) << static_cast<int>(buf[i]);
	return ss.str();
}

std::string parseSid(const std::string &cookieHeader)
{
	size_t pos = 0;
	while (pos < cookieHeader.size())
	{
		while (pos < cookieHeader.size() && (cookieHeader[pos] == ' ' || cookieHeader[pos] == '\t'))
			++pos;

		size_t semi = cookieHeader.find(';', pos);
		std::string pair = cookieHeader.substr(pos, semi == std::string::npos ? std::string::npos : semi - pos);

		size_t eq = pair.find('=');
		if (eq != std::string::npos)
		{
			std::string name = pair.substr(0, eq);
			// trim
			size_t ns = name.find_first_not_of(" \t");
			size_t ne = name.find_last_not_of(" \t");
			if (ns != std::string::npos)
				name = name.substr(ns, ne - ns + 1);
			if (name == "sid")
				return pair.substr(eq + 1);
		}

		if (semi == std::string::npos)
			break;
		pos = semi + 1;
	}
	return "";
}

std::map<std::string, std::string> load(const std::string &sid)
{
	std::map<std::string, std::string> data;
	if (!validSid(sid))
		return data;

	std::ifstream f((std::string(SESSION_DIR) + "/" + sid).c_str());
	if (!f)
		return data;

	std::string line;
	std::string last_seen_str;
	while (std::getline(f, line))
	{
		size_t eq = line.find('=');
		if (eq == std::string::npos)
			continue;
		std::string key = line.substr(0, eq);
		if (key == "last_seen")
		{
			last_seen_str = line.substr(eq + 1);
			continue;
		}
		if (key == "created")
			continue;
		data[key] = line.substr(eq + 1);
	}
	f.close();

	if (!last_seen_str.empty())
	{
		time_t last_seen = static_cast<time_t>(atol(last_seen_str.c_str()));
		if (std::time(NULL) - last_seen > SESSION_TIMEOUT)
		{
			std::remove((std::string(SESSION_DIR) + "/" + sid).c_str());
			return std::map<std::string, std::string>();
		}
	}

	return data;
}

// Write session data, preserving the original creation timestamp.
void save(const std::string &sid, const std::map<std::string, std::string> &data)
{
	if (!validSid(sid))
		return;
	ensureDir();

	// Preserve the original "created" value if the file already exists.
	std::string created;
	{
		std::ifstream rf((std::string(SESSION_DIR) + "/" + sid).c_str());
		std::string line;
		while (std::getline(rf, line))
		{
			if (line.substr(0, 8) == "created=")
			{
				created = line.substr(8);
				break;
			}
		}
	}

	std::ofstream f((std::string(SESSION_DIR) + "/" + sid).c_str());
	if (!f)
		return;

	if (created.empty())
	{
		std::ostringstream ts;
		ts << std::time(NULL);
		created = ts.str();
	}
	std::ostringstream ls;
	ls << std::time(NULL);

	f << "created=" << created << "\n";
	f << "last_seen=" << ls.str() << "\n";
	for (std::map<std::string, std::string>::const_iterator it = data.begin(); it != data.end(); ++it)
		f << it->first << "=" << it->second << "\n";
}

void destroy(const std::string &sid)
{
	if (!validSid(sid))
		return;
	std::remove((std::string(SESSION_DIR) + "/" + sid).c_str());
}

}
