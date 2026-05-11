#pragma once

#include <string>					// string
#include <fstream>					// ofstream

class HttpRequestBody
{

public:

	HttpRequestBody();
	~HttpRequestBody();

	void init(int fd);
	void init(const std::string &uploadDir, const std::string &boundary);	// multipart upload
	void init(const std::string &uploadDir);								// raw upload

	HttpRequestBody &operator+=(const std::string &chunk);

	size_t	size()	const;
	bool	empty()	const;
	void	reset();

private:

	HttpRequestBody(const HttpRequestBody &);
	HttpRequestBody &operator=(const HttpRequestBody &);

	enum UploadState { INACTIVE, WAITING_PART_HEADERS, STREAMING, DONE };

	void processMultipart(const std::string &chunk);
	void openFile(const std::string &partHeaders);

	// pipe case
	std::string	data_;
	size_t		offset_;

	// file/multipart case
	std::ofstream	*file_;
	std::string		uploadDir_;
	std::string		boundary_;	// empty = raw upload
	std::string		headerBuf_;	// accumulates until part headers end
	UploadState		uploadState_;

	// common between file and pipe case
	size_t		size_;
	int			fd_;

};
