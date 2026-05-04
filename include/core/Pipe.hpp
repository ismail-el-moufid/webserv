#pragma once























class Pipe
{

public:

	Pipe();
	~Pipe();

	int readFd() const;
	int writeFd() const;

	void closeWrite();
	void closeRead();

private:

	// Non-copyable
	Pipe(const Pipe&);
	// Non-assignable
	Pipe &operator=(const Pipe&);

	int readFd_;
	int writeFd_;

};
