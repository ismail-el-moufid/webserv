#pragma once

#include "core/Socket.hpp"			// Socket
#include "core/Pipe.hpp"			// Pipe

#include <vector>					// vector
#include <poll.h>					// struct pollfd










typedef void* FdContext;
typedef void* Invoker;


class Poller
{

public:

	Poller();

	// fdContext: caller context passed back in dispatcher, avoids fd-to-object lookup
	void add(Socket &socket, int events, FdContext fdContext);
	void add(Pipe &pipe, int events, FdContext fdContext);

	void mod(Socket &socket, int events);

	void remove(Socket &socket);
	void remove(Pipe &pipe);

	// timeout_ms: -1 = infinite
	void waitAndDispatch(int timeout_ms, void (*dispatcher)(FdContext fdContext, int revents, Invoker invoker), Invoker invoker);

private:

	// Non-copyable
	Poller(const Poller&);
	// Non-assignable
	Poller &operator=(const Poller&);

	void add(int fd, int events, FdContext fdContext);
	void mod(int fd, int events);
	void remove(int fd);

	std::vector<struct pollfd>	pfds_;			// passed directly to poll()
	std::vector<FdContext>		fdContexts_;	// parallel to pfds_
	std::vector<int>			fd_to_index_;	// fd_to_index_[fd] → index in pfds_ and fdContexts_

	// temporary storage for dispatch
	std::vector<FdContext>	ready_fdContexts_;
	std::vector<int>		ready_revents_;

};
