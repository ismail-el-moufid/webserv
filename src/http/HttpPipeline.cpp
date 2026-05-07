#include "http/HttpPipeline.hpp"
#include "utils/MimeUtils.hpp"	// MimeUtils::mimeByPath
#include "utils/StringUtils.hpp"

#include <dirent.h>				// opendir, readdir, closedir
#include <fstream>				// ifstream
#include <iomanip>				// setw
#include <sstream>				// ostringstream
#include <sys/stat.h>			// stat, S_ISDIR
#include <iostream>				// cout, ios, left

namespace
{

const VirtualHost *resolveVhost(const std::vector<VirtualHost *> &candidates, const std::string &host)
{
	for (size_t i = 0; i < candidates.size(); ++i)
		if (candidates.at(i)->hasName(host))
			return candidates.at(i);

	return candidates.empty() ? NULL : candidates.at(0);
}

const Route *resolveRoute(const VirtualHost &vhost, const std::string &uri)
{
	const Route *best	= NULL;
	size_t		bestLen = 0;

	const std::vector<Route> &routes = vhost.routes();
	for (size_t i = 0; i < routes.size(); ++i)
	{
		const std::string &path = routes.at(i).path();

		if (uri.compare(0, path.size(), path) != 0)
			continue;

		if (path != "/" && uri.size() > path.size() && uri.at(path.size()) != '/')
			continue;

		if (path.size() > bestLen)
		{
			best	= &routes.at(i);
			bestLen	= path.size();
		}
	}

	return best;
}

std::string resolveFilePath(const Route &route, const std::string &uri)
{
	std::string path = uri.substr(0, uri.find('?'));
	
	std::string root = route.root();
	
	// Remove trailing slash from root if it exists
	if (!root.empty() && root[root.size() - 1] == '/')
		root.erase(root.size() - 1);
	
	// Ensure path starts with a slash
	if (!path.empty() && path[0] != '/')
		path = "/" + path;

	return root + path;
}

bool resolveIndex(const Route &route, const std::string &dirPath, std::string &resolvedFilePath)
{
	const std::vector<std::string> &indexes = route.indexFiles();
	for (size_t i = 0; i < indexes.size(); ++i)
	{
		std::string candidate = dirPath + "/" + indexes.at(i);
		struct stat st;
		if (stat(candidate.c_str(), &st) == 0 && !S_ISDIR(st.st_mode))
			return (resolvedFilePath = candidate, true);
	}
	return false;
}

void appendDirEntry(std::ostringstream &body, const std::string &path, const std::string &name)
{
	struct stat st;
	if (stat((path + "/" + name).c_str(), &st) != 0)
		return ;

	bool		isDir	= S_ISDIR(st.st_mode);
	std::string	display	= name + (isDir ? "/" : "");

	char displayTime[32];
	strftime(displayTime, sizeof(displayTime), "%d-%b-%Y %H:%M", localtime(&st.st_mtime));

	body << "<a href=\"" << display << "\">"
		<< std::left << std::setw(50) << display << "</a>"
		<< std::setw(20) << displayTime
		<< std::setw(20) << (isDir ? "-" : "")
		<< (isDir ? "" : StringUtils::toString(st.st_size))
		<< "\r\n";
}

HttpResponse dirListing(const HttpRequest &request, const std::string &path)
{
	DIR *dir = opendir(path.c_str());
	if (!dir)
		return HttpPipeline::errorResponse(request, HttpStatus::Forbidden);

	std::ostringstream body;
	body << "<html><head><title>Index of " << request.uri() << "</title></head><body><h1>Index of "
		<< request.uri() << "</h1><hr><pre><a href=\"../\">../</a>\n";

	struct dirent *content;
	while ((content = readdir(dir)) != NULL)
	{
		std::string name(content->d_name);
		if (name == "." || name == "..")
			continue ;
		appendDirEntry(body, path, name);
	}
	closedir(dir);
	body << "</pre><hr></body>\r\n</html>\r\n";

	HttpResponse res;
	res.setStatus(HttpStatus::OK);
	res.setContentType("text/html");
	res.setBody(body.str());
	return res;
}

HttpResponse staticResponse(const HttpRequest &request)
{
	const Route	&route		= *request.route;
	std::string	filePath	= resolveFilePath(route, request.uri());

	struct stat st;
	if (stat(filePath.c_str(), &st) != 0)
		return HttpPipeline::errorResponse(request, HttpStatus::NotFound);

	if (S_ISDIR(st.st_mode))
	{
		if (request.uri().back() != '/')
			return HttpResponse::HttpResponseRedirect(HttpStatus::MovedPermanently, request.uri() + "/");
		std::string index;

		if (resolveIndex(route, filePath, index))
			filePath = index;
		else if (route.autoIndexed())
			return dirListing(request, filePath);
		else
			return HttpPipeline::errorResponse(request, HttpStatus::Forbidden);
	}

	std::ifstream file(filePath.c_str(), std::ios::binary);
	if (!file)
		return HttpPipeline::errorResponse(request, HttpStatus::Forbidden);

	std::ostringstream buf;
	buf << file.rdbuf();

	HttpResponse res;
	res.setStatus(HttpStatus::OK);
	res.setContentType(MimeUtils::mimeByPath(filePath));
	res.setBody(buf.str());

	return res;
}

}

namespace HttpPipeline
{

void resolve(HttpRequest &request, const ListenEndpoints &endpoints, const Interface &iface)
{
	request.vhost = NULL;
	request.route = NULL;

	ListenEndpoints::const_iterator it = endpoints.find(iface);
	if (it == endpoints.end() || it->second.empty())
		return ;

	if (!(request.vhost = resolveVhost(it->second, request.host())))
		return ;

	const std::string &uri = request.uri();
	request.route = resolveRoute(*request.vhost, uri.substr(0, uri.find('?')));
}

HttpResponse buildResponse(const HttpRequest &request)
{
	if (request.erroneous())
		return errorResponse(request, request.errorCode());

	if (!request.route)
		return errorResponse(request, HttpStatus::NotFound);

	const Route &route = *request.route;

	if (route.maxBodySize() > 0 && request.contentLength() > route.maxBodySize())
		return errorResponse(request, HttpStatus::ContentTooLarge);

	if (route.redirected())
		return HttpResponse::HttpResponseRedirect(route.redirectCode(), route.redirectPage());

	// CGI
	if (request.hasCgi())
		return errorResponse(request, HttpStatus::InternalServerError);

	// Upload — TODO
	if (!route.upload().empty() && (request.method() == "POST" || request.method() == "PUT"))
		return errorResponse(request, HttpStatus::NotImplemented);

	// Static
	return staticResponse(request);
}

void logRequest(const HttpRequest &request, const HttpResponse &response, const Interface &iface, size_t bytesSent)
{
	std::string ip, port;
	NetworkUtils::extractIPPort(iface, ip, port);

	std::cout << ip << " - - " << StringUtils::currentTime() 
				<< " \"" << request.method() << " " << request.uri() << " " << request.version() << "\""
				<< " " << static_cast<int>(response.statusCode()) << " " << bytesSent
				<< "\n";
}

HttpResponse errorResponse(const HttpRequest &request, HttpStatus::Code code)
{
	std::string pagePath;

	if (request.route)
	{
		std::map<HttpStatus::Code, std::string>::const_iterator it = request.route->errorPages().find(code);
		if (it != request.route->errorPages().end())
			pagePath = it->second;
	}
	if (pagePath.empty() && request.vhost)
	{
		std::map<HttpStatus::Code, std::string>::const_iterator it = request.vhost->errorPages().find(code);
		if (it != request.vhost->errorPages().end())
			pagePath = it->second;
	}

	std::ifstream file(pagePath.c_str());
	std::ostringstream buf;
	if (file)
		buf << file.rdbuf();
	std::string body = buf.str();

	if (body.empty())
		return HttpResponse::HttpErrorResponse(code);

	HttpResponse res;
	res.setStatus(code);
	res.setContentType("text/html");
	res.setBody(body);
	return res;
}
HttpResponse buildResponseFromRaw(const HttpRequest &request, const std::string &rawOutput)
{
	size_t	headerEnd		= rawOutput.find("\r\n\r\n");
	size_t	lineBreakSize	= 4;
	if (headerEnd == std::string::npos)
	{
		headerEnd		= rawOutput.find("\n\n");
		lineBreakSize	= 2;
	}
	if (headerEnd == std::string::npos)
		return errorResponse(request, HttpStatus::InternalServerError);

	std::string headerSection	= rawOutput.substr(0, headerEnd);
	std::string body			= rawOutput.substr(headerEnd + lineBreakSize);

	HttpResponse response;
	size_t pos = 0;
	while (pos < headerSection.size())
	{
		size_t lineEnd	= headerSection.find("\r\n", pos);
		size_t lineStep	= 2;
		if (lineEnd == std::string::npos)
		{
			lineEnd		= headerSection.find('\n', pos);
			lineStep	= 1;
		}
		std::string	line	= headerSection.substr(pos, lineEnd == std::string::npos ? std::string::npos : lineEnd - pos);
		size_t		colon	= line.find(':');
		if (colon != std::string::npos)
		{
			std::string name	= StringUtils::trim(line.substr(0, colon));
			std::string value	= StringUtils::trim(line.substr(colon + 1));
			if (name == "Status")
			{
				char *end;
				long code = std::strtol(value.c_str(), &end, 10);
				if (end != value.c_str() && *end == ' ' && HttpStatus::isValidCode(std::string(value.c_str(), 3)))
					response.setStatus(static_cast<HttpStatus::Code>(code));
				else
					response.setStatus(HttpStatus::InternalServerError);
			}
			else
				response.setHeader(name, value);
		}
		if (lineEnd == std::string::npos)
			break ;
		pos = lineEnd + lineStep;
	}

	if (HttpStatus::isErrorCode(response.statusCode()) && body.empty())
		return errorResponse(request, response.statusCode());

	response.setBody(body);
	return response;
}

}
