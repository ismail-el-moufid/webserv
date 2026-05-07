#include "core/IOReactor.hpp"

#include <cstddef>

#define REGISTERED(fd) ((fd) != -1 && (size_t)(fd) < fd_to_index_.size() && fd_to_index_.at(fd) != -1)

IOReactor::IOReactor(time_t timeout) : fd_to_index_(10240, -1), timeout_(timeout) {}
IOReactor::~IOReactor() {}

void IOReactor::add(int fd, int events, IPollable &pollable)
{
	if (fd == -1 || events == 0)
		return ;
	if ((size_t)fd >= fd_to_index_.size())
		fd_to_index_.resize(fd + 1, -1);
	if (REGISTERED(fd))
	{
		mod(fd, events);
		return ;
	}

	struct pollfd pfd;
	pfd.fd		= fd;
	pfd.events	= events;
	pfd.revents	= 0;

	pfds_.push_back(pfd);
	pollables_.push_back(&pollable);
	fd_to_index_.at(fd) = pfds_.size() - 1;
}

void IOReactor::mod(int fd, int events)
{
	if (!REGISTERED(fd))
		return ;

	pfds_.at(fd_to_index_.at(fd)).events = events;
}

void IOReactor::remove(int fd)
{
	if (!REGISTERED(fd))
		return ;

	const int idx = fd_to_index_.at(fd);
	if (idx != (int)pfds_.size() - 1)
	{
		const int last_fd			= pfds_.back().fd;
		pfds_.at(idx)			= pfds_.back();
		pollables_.at(idx)		= pollables_.back();
		fd_to_index_.at(last_fd)	= idx;
	}

	pfds_.pop_back();
	pollables_.pop_back();
	fd_to_index_.at(fd) = -1;
}

void IOReactor::add(IPollable &pollable, int events)
{
	if (events & POLLIN)	add(pollable.readFd(),	POLLIN,		pollable);
	if (events & POLLOUT)	add(pollable.writeFd(),	POLLOUT,	pollable);
}

void IOReactor::mod(IPollable &pollable, int events)
{
	if (pollable.readFd() != pollable.writeFd())
	{
		if (events & POLLIN)
			add(pollable.readFd(),	POLLIN,	pollable);
		else
			remove(pollable.readFd());

		if (events & POLLOUT)
			add(pollable.writeFd(), POLLOUT, pollable);
		else
			remove(pollable.writeFd());
	}
	else
		mod(pollable.readFd(), events);
}

void IOReactor::remove(IPollable &pollable)
{
	remove(pollable.readFd());
	if (pollable.writeFd() != pollable.readFd())
		remove(pollable.writeFd());
}

void IOReactor::waitAndDispatch(int timeToWaitInMS)
{
	if (!pfds_.empty() && poll(&pfds_.at(0), pfds_.size(), timeToWaitInMS) > 0)
	{
		ready_revents_.clear();
		ready_fds_.clear();

		// Snapshot fds that are ready
		for (size_t i = 0; i < pfds_.size(); ++i)
		{
			if (pfds_.at(i).revents != 0)
			{
				ready_revents_.push_back(pfds_.at(i).revents);
				ready_fds_.push_back(pfds_.at(i).fd);
			}
		}

		// Dispatch events for ready fds
		for (size_t i = 0; i < ready_fds_.size(); ++i)
		{
			if (!REGISTERED(ready_fds_.at(i)))
				continue;
			
			IPollable *pol = pollables_.at(fd_to_index_.at(ready_fds_.at(i)));
			
			if (ready_revents_.at(i) & POLLIN)
				pol->onRead();
			
			// Re-check registration as onRead() might have deleted the object
			if (REGISTERED(ready_fds_.at(i)) && pollables_.at(fd_to_index_.at(ready_fds_.at(i))) == pol)
			{
				if (ready_revents_.at(i) & POLLOUT)
					pol->onWrite();
			}
		}
	}

	// Check for timeouts on inactive connections
	// We iterate backward because onTimeout() can call remove(), which uses swap-and-pop.
	// Backward iteration ensures that removing the current element (or any element after it)
	// does not affect the indices of the elements we still need to process.
	time_t now = time(NULL);
	for (size_t i = pfds_.size(); i > 0; --i)
	{
		size_t current_idx = i - 1;

		// Check if the fd at the current index was already processed in the ready loop
		bool was_ready = false;
		for (size_t j = 0; j < ready_fds_.size(); ++j)
		{
			if (ready_fds_.at(j) == pfds_.at(current_idx).fd)
			{
				was_ready = true;
				break;
			}
		}
		if (was_ready)
			continue;
		
		// If not ready, check for timeout. This is now safe.
		IPollable *pol = pollables_.at(current_idx);
		if (now - pol->lastActive() > timeout_)
		{
			pol->onTimeout();
		}
	}
}

bool IOReactor::empty() const { return pfds_.empty(); }
