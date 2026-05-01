#pragma once

#include "core/IPollable.hpp"		// IPollable






#include <vector>					// vector
#include <poll.h>					// struct pollfd
















class IOReactor
{

public:

	IOReactor();
	~IOReactor();

	void add(IPollable &pollable, int events);

	void mod(IPollable &pollable, int events);

	void remove(IPollable &pollable);

	// timeout_ms: -1 = infinite
	void waitAndDispatch(int timeout_ms);

private:

	// Non-copyable
	IOReactor(const IOReactor&);
	// Non-assignable
	IOReactor &operator=(const IOReactor&);

	void add(int fd, int events, IPollable &pollable);
	void mod(int fd, int events);
	void remove(int fd);

	std::vector<struct pollfd>	pfds_;			// passed directly to poll()
	std::vector<IPollable *>	pollables_;		// parallel to pfds_
	std::vector<int>			fd_to_index_;	// fd_to_index_[fd] → index in pfds_ and pollables_

	// temporary storage for dispatch
	std::vector<IPollable *>	ready_pollables_;
	std::vector<int>			ready_revents_;
	std::vector<int>			ready_fds_;

};
