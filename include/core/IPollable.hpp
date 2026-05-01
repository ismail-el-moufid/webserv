#pragma once


























class IOReactor;

class IPollable
{
public:

	IPollable(IOReactor &reactor) : reactor_(reactor) {}
	virtual ~IPollable() { }

	virtual int		readFd()		const = 0;
	virtual int		writeFd()	const = 0;

	virtual void	onRead()		= 0;	// mandatory for all the classes that inherit
	virtual void	onWrite()		{ }		// optional, since for some they don't need it

protected:

	IOReactor &reactor_;

};
