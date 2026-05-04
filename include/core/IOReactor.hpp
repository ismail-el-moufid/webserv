#pragma once

#include "core/IPollable.hpp"		// IPollable






#include <ctime>
#include <vector>					// vector
#include <poll.h>					// struct pollfd












class IOReactor
{

public:

	IOReactor(time_t timeout = 30);
	~IOReactor();

	void add(IPollable &pollable, int events);

	void mod(IPollable &pollable, int events);

	void remove(IPollable &pollable);

	bool empty() const;

	// timeout_ms: -1 = infinite
	void waitAndDispatch(int timeToWaitInMS);

private:

	IOReactor();
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

	time_t						timeout_; // inactivity threshold in seconds before onTimeout() is called

};
