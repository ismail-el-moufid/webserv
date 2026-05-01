#include "core/IOReactor.hpp"

#include <cstddef>

#define REGISTERED(fd) ((fd) != -1 && (size_t)(fd) < fd_to_index_.size() && fd_to_index_.at(fd) != -1)

IOReactor::IOReactor() : fd_to_index_(10240, -1) {}
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
	if (events & POLLIN)  add(pollable.readFd(),  POLLIN,  pollable);
	if (events & POLLOUT) add(pollable.writeFd(), POLLOUT, pollable);
}

void IOReactor::mod(IPollable &pollable, int events)
{
	if (pollable.readFd() != pollable.writeFd())
	{
		if (events & POLLIN)
			add(pollable.readFd(),  POLLIN,  pollable);
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

void IOReactor::waitAndDispatch(int timeout_ms)
{
	if (pfds_.empty() || poll(&pfds_.at(0), pfds_.size(), timeout_ms) <= 0)
		return ;

	ready_pollables_.clear();
	ready_revents_.clear();
	ready_fds_.clear();

	for (size_t i = 0; i < pfds_.size(); ++i)
		if (pfds_.at(i).revents != 0)
		{
			ready_pollables_.push_back(pollables_.at(i));
			ready_revents_.push_back(pfds_.at(i).revents);
			ready_fds_.push_back(pfds_.at(i).fd);
		}
	for (size_t i = 0; i < ready_pollables_.size(); ++i)
	{
		// a pollable with multiple fds may delete itself in onRead() if its write fd is also in the snapshot, skip it to avoid use-after-free
		if (ready_revents_.at(i) & POLLIN  && REGISTERED(ready_fds_.at(i)))
			ready_pollables_.at(i)->onRead();
		if (ready_revents_.at(i) & POLLOUT && REGISTERED(ready_fds_.at(i)))
			ready_pollables_.at(i)->onWrite();
	}
}
