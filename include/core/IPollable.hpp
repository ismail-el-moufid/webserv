#pragma once

#include <cstddef>					// NULL
#include <ctime>					// time, time_t




















class IOReactor;

class IPollable
{
public:

	IPollable(IOReactor &reactor) : reactor_(reactor), lastActive_(time(NULL)) {}
	virtual ~IPollable() { }

	virtual int		readFd()	const = 0;
	virtual int		writeFd()	const = 0;

	virtual void	onRead()		= 0;	// mandatory for all the classes that inherit
	virtual void	onWrite()		{ }		// optional, since for some they don't need it
	virtual void	onTimeout()		{ }		// optional, since for some they don't need it

	void	updateActivity()	{ lastActive_ = time(NULL); }
	time_t	lastActive()		const { return lastActive_; }

protected:

	IOReactor	&reactor_;
	time_t		lastActive_;

};
