#pragma once

#include "http/HttpStatusCodes.hpp"	// HttpStatus::Code


#include <cstddef>					// size_t
#include <string>					// string
#include <vector>					// vector
#include <map>						// map



#define InstallDir "/usr/local/webserv"
#define HtmlDir InstallDir "/html"







class Route
{

public:

	Route();

	static Route &applyDefaults(Route &route);

	const std::string								&path() const;
	size_t											maxBodySize() const;
	const std::vector<std::string>					&methods() const;
	const std::string								&root() const;
	bool											redirected() const;
	const std::string								&redirectPage() const;
	HttpStatus::Code								redirectCode() const;
	bool											autoIndexed() const;
	const std::vector<std::string>					&indexFiles() const;
	const std::map<HttpStatus::Code, std::string>	&errorPages() const;
	const std::map<std::string, std::string>		&cgis() const;
	const std::string								&upload() const;

	void	setPath(const std::string &path);
	void	setMaxBodySize(const std::string &maxBodySize);
	void	addMethod(const std::string &method);
	void	setRoot(const std::string &root);
	void	setRedirect(const std::string &statusCode, const std::string &page);
	void	setAutoIndex(const std::string &autoindex);
	void	addIndexFile(const std::string &indexFile);
	void	addCgi(const std::string &extension, const std::string &path);
	void	addErrorPage(const std::string &code, const std::string &page);
	void	setUpload(const std::string &upload);

	void	clearMethods();

private:

	size_t									maxBodySize_;
	std::string								path_;
	std::vector<std::string>				methods_;
	std::string								root_;
	bool									redirect_;
	std::string								redirectPage_;
	HttpStatus::Code						redirectCode_;
	bool									autoindex_;
	std::vector<std::string>				indexFiles_;
	std::map<HttpStatus::Code, std::string>	errorPages_;
	std::map<std::string, std::string>		cgis_;
	std::string								upload_;

};
