#pragma once




















class Pipe
{

public:

	Pipe();
	~Pipe();

	int getReadFd() const;
	int getWriteFd() const;

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
