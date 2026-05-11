#pragma once

#include <string>					// string
#include <netdb.h>					// addrinfo

typedef struct { sa_family_t family; sockaddr_storage addr; socklen_t addrlen; } Interface;

namespace NetworkUtils
{
	bool resolve(const std::string &address, const std::string &port, Interface &result);
	bool resolve(const std::string &address, const std::string &port, struct addrinfo *&results);
	bool resolve(const std::string &address, const std::string &port);
	bool resolve(const std::string &address_port);

	void extractIPPort(const Interface &iface, std::string &ip, std::string &port);

	struct InterfaceCompare { bool operator()(const Interface &a, const Interface &b) const; };
}

bool operator==(const Interface &a, const Interface &b); // used by find() internally
