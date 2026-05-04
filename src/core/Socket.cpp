#include "core/Socket.hpp"			// Socket
#include "utils/StringUtils.hpp"	// currentTime

#include <sys/socket.h>				// socket()
#include <netinet/in.h>				// AF_INET, SOCK_STREAM
#include <unistd.h>					// close()
#include <stdexcept>				// runtime_error
#include <fcntl.h>					// fcntl(), F_SETFD, F_SETFL, FD_CLOEXEC, O_NONBLOCK

void Socket::configureSocket()
{
	if (fd_ == -1)
		throw std::runtime_error(StringUtils::currentTime() + " [error] Invalid socket file descriptor");

	// Set the close-on-exec flag to prevent file descriptor leaks in child processes
	// Also set the socket to non-blocking mode
	if (fcntl(fd_, F_SETFD, FD_CLOEXEC) == -1 || fcntl(fd_, F_SETFL, O_NONBLOCK) == -1)
	{
		close(fd_);
		fd_ = -1;
		throw std::runtime_error(StringUtils::currentTime() + " [error] Failed to set FD_CLOEXEC and O_NONBLOCK flags on socket file descriptor");
	}
}

Socket::Socket() : fd_(socket(AF_INET, SOCK_STREAM, 0))
{
	configureSocket();
}

Socket::Socket(int fd) : fd_(fd)
{
	configureSocket();
}

int Socket::get() const
{
	return fd_;
}

Socket::~Socket()
{
	if (fd_ != -1) close(fd_);
}
