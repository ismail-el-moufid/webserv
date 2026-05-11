#include "core/Pipe.hpp"			// Pipe
#include "utils/StringUtils.hpp"	// currentTime

#include <unistd.h>					// pipe(), close()
#include <stdexcept>				// runtime_error
#include <fcntl.h>					// fcntl(), F_SETFD, F_SETFL, FD_CLOEXEC, O_NONBLOCK

void Pipe::closeRead()
{
	if (readFd_ != -1)
		close(readFd_);
	readFd_ = -1;
}
void Pipe::closeWrite()
{
	if (writeFd_ != -1)
		close(writeFd_);
	writeFd_ = -1;
}
#include <iostream>
Pipe::Pipe() : readFd_(-1), writeFd_(-1)
{
	std::cout << StringUtils::currentTime() << " [info] Creating pipe\n";
	int fds[2];
	if (pipe(fds) == -1)
		throw std::runtime_error(StringUtils::currentTime() + " [error] Failed to create pipe");

	if (fcntl(fds[0], F_SETFD, FD_CLOEXEC) == -1 || fcntl(fds[1], F_SETFD, FD_CLOEXEC) == -1
		|| fcntl(fds[0], F_SETFL, O_NONBLOCK) == -1 || fcntl(fds[1], F_SETFL, O_NONBLOCK) == -1)
	{
		close(fds[0]);
		close(fds[1]);
		throw std::runtime_error(StringUtils::currentTime() + " [error] Failed to set FD_CLOEXEC and O_NONBLOCK flags on pipe file descriptors");
	}

	readFd_		= fds[0];
	writeFd_	= fds[1];
}
Pipe::~Pipe()
{
	if (readFd_		!= -1) close(readFd_);
	if (writeFd_	!= -1) close(writeFd_);
}

int Pipe::readFd() const
{
	return readFd_;
}

int Pipe::writeFd() const
{
	return writeFd_;
}
