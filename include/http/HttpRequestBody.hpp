#pragma once

#include <string>					// string
#include <fstream>					// ofstream

class HttpRequestBody
{

public:

	HttpRequestBody();
	~HttpRequestBody();

	void init(int fd);
	void initRaw(const std::string &uploadDir, int &errorCode);
	void initRaw(const std::string &uploadDir, const std::string &filename, int &errorCode);
	void initMultipart(const std::string &uploadDir, const std::string &boundary, int &errorCode);

	HttpRequestBody &operator+=(const std::string &chunk);

	int		drain();

	size_t	size()	const;
	bool	empty()	const;
	void	reset();

private:

	HttpRequestBody(const HttpRequestBody &);
	HttpRequestBody &operator=(const HttpRequestBody &);

	enum UploadState { INACTIVE, WAITING_PART_HEADERS, STREAMING, DONE };

	void processMultipart(const std::string &chunk);
	void openFile(const std::string &partHeaders);

	// pointer to HttpRequest's error code
	int *errorCodePtr_;

	bool errorOccurred_;

	// pipe case
	std::string	data_;

	// file/multipart case
	std::ofstream	*file_;
	std::string		uploadDir_;
	std::string		boundary_;	// empty = raw upload
	std::string		headerBuf_;	// accumulates until part headers end
	UploadState		uploadState_;

	// common between file and pipe case
	size_t		size_;
	int			fd_;
	size_t		offset_;

};
