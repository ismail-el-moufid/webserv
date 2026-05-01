#include "core/ListeningSocket.hpp"
#include "core/Client.hpp"
#include "utils/NetworkUtils.hpp"

#include <sys/socket.h>
#include <netdb.h>
#include <stdexcept>

ListeningSocket::ListeningSocket(const Interface &iface, IOReactor &reactor) : Socket(), IPollable(reactor), iface_(iface)
{
	int reuse = 1;
	if (setsockopt(get(), SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1)
		throw std::runtime_error("Failed to set SO_REUSEADDR on socket");
	if (bind(get(), reinterpret_cast<const struct sockaddr *>(&iface.addr), iface.addrlen) == -1)
	{
		std::string ip, port;
		NetworkUtils::extractIPPort(iface, ip, port);
		throw std::runtime_error("Failed to bind socket to address '" + ip + ":" + port + "'");
	}
	if (listen(get(), SOMAXCONN) == -1)
		throw std::runtime_error("Failed to listen on socket");

	reactor_.add(*this, POLLIN);
}

int ListeningSocket::readFd()  const { return Socket::get(); }
int ListeningSocket::writeFd() const { return Socket::get(); }

void ListeningSocket::onRead()
{
	int fd = accept(get(), NULL, NULL);
	if (fd == -1)
		return ;
	reactor_.add(*new Client(fd, iface_, reactor_), POLLIN);
}