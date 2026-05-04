#pragma once

#include "config/Config.hpp"		// ListenEndpoints
#include "core/IPollable.hpp"		// IPollable
#include "core/Socket.hpp"			// Socket
#include "core/IOReactor.hpp"		// IOReactor
#include "utils/NetworkUtils.hpp"	// Interface

















class ListeningSocket : public Socket, public IPollable
{

public:

	ListeningSocket(const Interface &iface, IOReactor &reactor, const ListenEndpoints &endpoints);

	int	readFd() const;
	int	writeFd() const;

	void onRead();

private:

	ListeningSocket(const ListeningSocket &);
	ListeningSocket &operator=(const ListeningSocket &);

	Interface iface_;
	const ListenEndpoints &endpoints_;
};
