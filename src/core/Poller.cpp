#include "core/Poller.hpp"			// Poller

#define VALID(fd) ((fd) != -1 && (size_t)(fd) < fd_to_index_.size() && fd_to_index_.at(fd) != -1)

Poller::Poller() : fd_to_index_(10240, -1) {}

void Poller::add(int fd, int events, FdContext fdContext)
{
	if (fd == -1 || events == 0)
		return ;

	if ((size_t)fd >= fd_to_index_.size())
		fd_to_index_.resize(fd + 1, -1);

	struct pollfd pfd;
	pfd.fd		= fd;
	pfd.events	= events;
	pfd.revents	= 0;

	pfds_.push_back(pfd);
	fdContexts_.push_back(fdContext);

	fd_to_index_.at(fd) = pfds_.size() - 1;
}

void Poller::mod(int fd, int events)
{
	if (!VALID(fd))
		return ;

	pfds_.at(fd_to_index_.at(fd)).events = events;
}

void Poller::remove(int fd)
{
	if (!VALID(fd))
		return ;

	const int idx = fd_to_index_.at(fd);

	if (idx != (int)pfds_.size() - 1)
	{
		const int last_fd		= pfds_.back().fd;
		pfds_.at(idx)				= pfds_.back();
		fdContexts_.at(idx)		= fdContexts_.back();
		fd_to_index_.at(last_fd)	= idx;
	}

	pfds_.pop_back();
	fdContexts_.pop_back();
	fd_to_index_.at(fd) = -1;
}

void Poller::add(Socket &socket, int events, FdContext fdContext)	{ add(socket.get(), events, fdContext); }
void Poller::mod(Socket &socket, int events)						{ mod(socket.get(), events); }
void Poller::remove(Socket &socket)									{ remove(socket.get()); }

void Poller::add(Pipe &pipe, int events, FdContext fdContext)
{
	add(pipe.getReadFd(),	events & POLLIN,	fdContext);
	add(pipe.getWriteFd(),	events & POLLOUT,	fdContext);
}

void Poller::remove(Pipe &pipe)
{
	remove(pipe.getReadFd());
	remove(pipe.getWriteFd());
}

// This mess is because you can't pass a member function pointer as arg in C++98
// fdContext: context associated with the fd
// invoker: pointer to the object calling waitAndDispatch (this pointer in the caller)
void Poller::waitAndDispatch(int timeout_ms, void (*dispatcher)(FdContext fdContext, int revents, void *invoker), Invoker invoker)
{
	if (pfds_.empty() || poll(&pfds_.at(0), pfds_.size(), timeout_ms) <= 0)
		return ;

	ready_fdContexts_.clear();
	ready_revents_.clear();
	for (size_t i = 0; i < pfds_.size(); ++i)
		if (pfds_.at(i).revents != 0)
		{
			ready_fdContexts_.push_back(fdContexts_.at(i));
			ready_revents_.push_back(pfds_.at(i).revents);
		}

	for (size_t i = 0; i < ready_fdContexts_.size(); ++i)
		dispatcher(ready_fdContexts_.at(i), ready_revents_.at(i), invoker);
}
