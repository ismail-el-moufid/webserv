#include "utils/NetworkUtils.hpp"
#include "utils/StringUtils.hpp"

#include <cstring>
#include <string>
#include <netdb.h>
#include <sys/socket.h>

namespace NetworkUtils
{

bool resolve(const std::string &address, const std::string &port, struct addrinfo *&results)
{
	struct addrinfo hints	= {};
	hints.ai_family			= AF_INET;
	hints.ai_socktype		= SOCK_STREAM;
	hints.ai_flags			= AI_PASSIVE;
	return getaddrinfo(address.empty() ? NULL : address.c_str(), port.c_str(), &hints, &results) == 0;
}

bool resolve(const std::string &address, const std::string &port, Interface &result)
{
	struct addrinfo hints	= {};
	hints.ai_family			= AF_INET;
	hints.ai_socktype		= SOCK_STREAM;
	hints.ai_flags			= AI_PASSIVE;

	struct addrinfo *results;
	if (getaddrinfo(address.empty() ? NULL : address.c_str(), port.c_str(), &hints, &results) != 0)
		return false;

	result.family	= results->ai_family;
	result.addrlen	= results->ai_addrlen;
	std::memcpy(&result.addr, results->ai_addr, results->ai_addrlen);
	freeaddrinfo(results);
	return true;
}


bool InterfaceCompare::operator()(const Interface &a, const Interface &b) const
{
	if (a.family  != b.family)  return a.family  < b.family;
	if (a.addrlen != b.addrlen) return a.addrlen < b.addrlen;
	return std::memcmp(&a.addr, &b.addr, a.addrlen) < 0;
}

bool resolve(const std::string &address, const std::string &port)
{
	struct addrinfo *results;
	bool ok = resolve(address, port, results);
	if (ok)
		freeaddrinfo(results);
	return ok;
}

bool resolve(const std::string &address_port)
{
	size_t colon = address_port.rfind(':');
	if (colon == std::string::npos)
		return false;
	return resolve(address_port.substr(0, colon), address_port.substr(colon + 1));
}

void extractIPPort(const Interface &iface, std::string &ip, std::string &port)
{
	const struct sockaddr_in *sin =
		reinterpret_cast<const struct sockaddr_in *>(&iface.addr);

	unsigned long addr = ntohl(sin->sin_addr.s_addr);
	ip =  StringUtils::toString((addr >> 24)	& 0xFF) + "."
		+ StringUtils::toString((addr >> 16)	& 0xFF) + "."
		+ StringUtils::toString((addr >>  8)	& 0xFF) + "."
		+ StringUtils::toString( addr		& 0xFF);
	port = StringUtils::toString(ntohs(sin->sin_port));
}

}

bool operator==(const Interface &a, const Interface &b)
{
	return a.family		== b.family
		&& a.addrlen	== b.addrlen
		&& std::memcmp(&a.addr, &b.addr, a.addrlen) == 0;
}
