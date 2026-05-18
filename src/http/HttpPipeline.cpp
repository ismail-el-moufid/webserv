#include "http/HttpPipeline.hpp"	// buildResponse, buildResponseFromRaw, errorResponse, logRequest, resolve
#include "utils/MimeUtils.hpp"		// MimeUtils::mimeByPath

#include <algorithm>				// find
#include "utils/StringUtils.hpp"	// currentTime, toString, trim
#include "http/HttpStatusCodes.hpp"	// Code, ContentTooLarge, Created, Forbidden, InternalServerError, OK, MovedPermanently, NotFound, NoContent, isValidCode, toCode, isErrorCode 

#include <dirent.h>				// opendir, readdir, closedir
#include <fstream>				// ifstream
#include <iomanip>				// setw
#include <sstream>				// ostringstream
#include <sys/stat.h>			// stat, S_ISDIR
#include <iostream>				// cout, ios, left
#include <cstdlib>				// strtol
#include <unistd.h>				// access

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
	size_t		bestLen	= 0;

	const std::vector<Route> &routes = vhost.routes();
	for (size_t i = 0; i < routes.size(); ++i)
	{
		const std::string &path = routes.at(i).path();

		if (uri.compare(0, path.size(), path) != 0)
			continue ;

		if (path != "/" && uri.size() > path.size() && uri.at(path.size()) != '/')
			continue ;

		if (path.size() > bestLen)
		{
			best	= &routes.at(i);
			bestLen	= path.size();
		}
	}

	return best;
}

HttpResponse dirListing(const HttpRequest &request, const std::string &path)
{
	DIR *dir = opendir(path.c_str());
	if (!dir)
		return HttpPipeline::errorResponse(request, HttpStatus::Forbidden);

	std::ostringstream body;
	body	<< "<html><head><title>Index of " << request.uri().path << "</title></head><body><h1>Index of "
			<< request.uri().path << "</h1><hr><pre><a href=\"../\">../</a>\n";

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL)
	{
		std::string name(entry->d_name);
		if (name == "." || name == "..")
			continue ;
		struct stat st;
		if (stat((path + "/" + name).c_str(), &st) != 0)
			continue ;
		bool		isDir	= S_ISDIR(st.st_mode);
		std::string	display	= name + (isDir ? "/" : "");
		char t[32];
		strftime(t, sizeof(t), "%d-%b-%Y %H:%M", localtime(&st.st_mtime));
		body	<< "<a href=\"" << display << "\">"
				<< std::left << std::setw(50) << display << "</a>"
				<< std::setw(20) << t
				<< (isDir ? "-" : StringUtils::toString(st.st_size)) << "\r\n";
	}
	closedir(dir);
	body << "</pre><hr></body>\r\n</html>\r\n";

	HttpResponse res;
	res.setStatus(HttpStatus::OK);
	res.setContentType("text/html");
	res.setBody(body.str());
	return res;
}

HttpResponse serveFile(const HttpRequest &request, const std::string &filePath)
{
	struct stat st;
	if (stat(filePath.c_str(), &st) != 0)
		return HttpPipeline::errorResponse(request, HttpStatus::Forbidden);

	HttpResponse res;
	res.setStatus(HttpStatus::OK);
	res.setContentType(MimeUtils::mimeByPath(filePath));
	res.setFile(filePath, st.st_size);
	return res;
}

HttpResponse staticResponse(const HttpRequest &request)
{
	const Route &route	= *request.route;
	std::string root	= route.root();
	if (!root.empty() && root[root.size() - 1] == '/')
		root.erase(root.size() - 1);
	std::string filePath = root + request.uri().path;

	struct stat st;
	if (stat(filePath.c_str(), &st) != 0)
		return HttpPipeline::errorResponse(request, HttpStatus::NotFound);

	if (!S_ISDIR(st.st_mode))
		return serveFile(request, filePath);

	if (request.uri().path[request.uri().path.size() - 1] != '/')
		return HttpResponse::HttpResponseRedirect(HttpStatus::MovedPermanently,
			request.uri().path + "/" + (request.uri().hasQuery ? "?" + request.uri().query : ""));

	const std::vector<std::string> &indexes = route.indexFiles();
	for (size_t i = 0; i < indexes.size(); ++i)
	{
		std::string candidate = filePath + "/" + indexes.at(i);
		if (stat(candidate.c_str(), &st) == 0 && !S_ISDIR(st.st_mode))
			return serveFile(request, candidate);
	}

	if (route.autoIndexed())
		return dirListing(request, filePath);
	return HttpPipeline::errorResponse(request, HttpStatus::Forbidden);
}

HttpResponse deleteResponse(const HttpRequest &request)
{
	std::string root = request.route->root();
	if (!root.empty() && root[root.size() - 1] == '/')
		root.erase(root.size() - 1);

	std::string filename = request.uri().path;
	size_t slash = filename.find_last_of("/\\");
	if (slash != std::string::npos)
		filename = filename.substr(slash + 1);
	std::string path = root + request.route->path() + "/" + filename;
	struct stat path_stat;

	if (stat(path.c_str(), &path_stat) != 0)
		return HttpPipeline::errorResponse(request, HttpStatus::NotFound);
	if (S_ISDIR(path_stat.st_mode))
		return HttpPipeline::errorResponse(request, HttpStatus::Forbidden);
	if (access(path.c_str(), W_OK) != 0)
		return HttpPipeline::errorResponse(request, HttpStatus::Forbidden);
	if (std::remove(path.c_str()) == 0)
	{
		HttpResponse res;
		res.setStatus(HttpStatus::NoContent);
		return res;
	}
	else
		return HttpPipeline::errorResponse(request, HttpStatus::InternalServerError);
}

bool invalidRequest(const HttpRequest &request, HttpResponse &out, std::string *effectiveMethod = NULL)
{
	if (request.erroneous())
		return (out = HttpPipeline::errorResponse(request, request.errorCode())), true;

	if (!request.route)
		return (out = HttpPipeline::errorResponse(request, HttpStatus::NotFound)), true;

	if (request.route->maxBodySize() > 0 && request.contentLength() > request.route->maxBodySize())
		return (out = HttpPipeline::errorResponse(request, HttpStatus::ContentTooLarge)), true;

	// Redirects bypass method checking — a redirect is not a resource.
	if (request.route->redirected())
		return (out = HttpResponse::HttpResponseRedirect(request.route->redirectCode(), request.route->redirectPage())), true;

	// Current implementation only allows HEAD for GET routes.
	const std::string	&method		= request.method();
	const std::string	effective	= (method == "HEAD") ? "GET" : method;
	if (effectiveMethod)
		*effectiveMethod = effective;

	if (find(request.route->allowedMethods().begin(), request.route->allowedMethods().end(), effective) == request.route->allowedMethods().end())
		return (out = HttpPipeline::errorResponse(request, HttpStatus::MethodNotAllowed)), true;

	return false;
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
	request.route = resolveRoute(*request.vhost, request.uri().path);
}

HttpResponse buildResponse(const HttpRequest &request)
{
	HttpResponse errorHappened;
	std::string method;
	if (invalidRequest(request, errorHappened, &method))
		return errorHappened;

	if (!request.route->upload().empty() && (method == "POST" || method == "PUT")) {
		return HttpResponse::HttpResponseBuilder(HttpStatus::Created, "{\"status\":\"ok\"}", "application/json");
	}

	if (method == "DELETE")
		return deleteResponse(request);

	if (method == "GET")
	{
		HttpResponse resp = staticResponse(request);
		if (request.method() == "HEAD" && !resp.hasFile())
			resp.setBody("");
		return resp;
	}

	return errorResponse(request, HttpStatus::NotImplemented);
}

HttpResponse buildResponse(const HttpRequest &request, const std::string &raw)
{
	HttpResponse errorHappened;
	if (invalidRequest(request, errorHappened))
		return errorHappened;

	size_t headerEnd = raw.find("\r\n\r\n"), step = 4;

	if (headerEnd == std::string::npos)
	{
		headerEnd	= raw.find("\n\n");
		step		= 2;
	}
	if (headerEnd == std::string::npos)
		return errorResponse(request, HttpStatus::InternalServerError);

	std::istringstream	ss(raw.substr(0, headerEnd));
	std::string			body = raw.substr(headerEnd + step);

	HttpResponse	response;
	std::string		line;

	while (std::getline(ss, line))
	{
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		size_t colon = line.find(':');
		if (colon == std::string::npos)
			continue ;
		std::string name	= StringUtils::trim(line.substr(0, colon));
		std::string value	= StringUtils::trim(line.substr(colon + 1));
		if (name == "Status")
		{
			char *end;
			std::strtol(value.c_str(), &end, 10);
			std::string code(value.c_str(), end - value.c_str());
			response.setStatus(HttpStatus::isValidCode(code)
				? HttpStatus::toCode(code)
				: HttpStatus::InternalServerError);
		}
		else if (name == "Set-Cookie")
			response.addCookie(value);
		else
			response.setHeader(name, value);
	}

	if (HttpStatus::isErrorCode(response.statusCode()) && body.empty())
		return errorResponse(request, response.statusCode());

	response.setBody(body);
	return response;
}

void logRequest(const HttpRequest &request, const HttpResponse &response, const Interface &iface, size_t bytesSent)
{
	std::string ip, port;
	NetworkUtils::extractIPPort(iface, ip, port);

	std::cout << ip << " - - " << StringUtils::currentTime() 
				<< " \"" << request.method() << " " << request.uri().uri << " " << request.version() << "\""
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

int prepareUploadRequest(HttpRequest &request)
{
	struct stat uploadDir;
	if (stat(request.route->upload().c_str(), &uploadDir) != 0 || !S_ISDIR(uploadDir.st_mode) || access(request.route->upload().c_str(), W_OK) == -1)
		return HttpStatus::InternalServerError;

	size_t boundaryPos = request.contentType().find("boundary=");
	if (boundaryPos != std::string::npos)
	{
		request.initBodyMultipart(request.route->upload() + request.route->path(), request.contentType().substr(boundaryPos + 9, request.contentType().find(';', boundaryPos + 9) - (boundaryPos + 9)));
		return 0;
	}
	// Use the URI basename as the filename so POST /uploads/foo.txt saves as foo.txt.
	std::string filename = StringUtils::uriBasename(request.uri().path);
	if (filename.empty())
		request.initBodyRaw(request.route->upload() + request.route->path());
	else
		request.initBodyRaw(request.route->upload() + request.route->path(), filename);
	return 0;
}

}
