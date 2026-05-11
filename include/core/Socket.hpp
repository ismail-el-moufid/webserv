#pragma once

class Socket
{

public:

	Socket();
	Socket(int fd);

	int get() const;

	~Socket();

private:

	// Non-copyable
	Socket(const Socket&);
	// Non-assignable
	Socket &operator=(const Socket&);

	void configureSocket();

	int fd_;

};
