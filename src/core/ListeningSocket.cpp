#include "core/ListeningSocket.hpp"
#include "core/Client.hpp"
#include "utils/NetworkUtils.hpp"
#include "utils/StringUtils.hpp"

#include <sys/socket.h>
#include <netdb.h>
#include <stdexcept>
#include <iostream>

ListeningSocket::ListeningSocket(const Interface &iface, IOReactor &reactor, const ListenEndpoints &endpoints) : Socket(), IPollable(reactor), iface_(iface), endpoints_(endpoints)
{
	int reuse = 1;
	if (setsockopt(get(), SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1)
		throw std::runtime_error(StringUtils::currentTime() + " [error] Failed to set SO_REUSEADDR on socket");

	std::string ip, port;
	NetworkUtils::extractIPPort(iface, ip, port);

	if (bind(get(), reinterpret_cast<const struct sockaddr *>(&iface.addr), iface.addrlen) == -1)
		throw std::runtime_error(StringUtils::currentTime() + " [error] Failed to bind socket to address '" + ip + ":" + port + "'");

	if (listen(get(), SOMAXCONN) == -1)
		throw std::runtime_error(StringUtils::currentTime() + " [error] Failed to listen on socket");

	std::cout << StringUtils::currentTime() << " listening on " << ip << ":" << port << "\n";

	reactor_.add(*this, POLLIN);
}

int ListeningSocket::readFd()	const { return Socket::get(); }
int ListeningSocket::writeFd()	const { return Socket::get(); }

void ListeningSocket::onRead()
{
	int fd;
	Interface clientIface;
	while ((fd = accept(get(), reinterpret_cast<struct sockaddr *>(&clientIface.addr), &clientIface.addrlen)) != -1)
	{
		try { reactor_.add(*new Client(fd, iface_, clientIface, reactor_, endpoints_), POLLIN); }
		catch (const std::exception &e) { std::cerr << StringUtils::currentTime() << " [error] " << e.what() << "\n"; }
	}
}
