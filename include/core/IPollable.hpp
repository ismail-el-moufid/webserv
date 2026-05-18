#pragma once

#include <ctime>					// time, time_t
#include <cstddef>					// NULL

class IOReactor;

class IPollable
{
public:

	IPollable(IOReactor &reactor) : reactor_(reactor), lastActive_(time(NULL)) {}
	virtual ~IPollable() { }

	virtual int		readFd()	const = 0;
	virtual int		writeFd()	const = 0;

	virtual void	onRead()		= 0;
	virtual void	onWrite()		{ }
	virtual void	onTimeout()		{ }
	virtual void	onShutdown()	{ };

	void	updateActivity()	{ lastActive_ = time(NULL); }
	time_t	lastActive()		const { return lastActive_; }

protected:

	IOReactor	&reactor_;
	time_t		lastActive_;

};
