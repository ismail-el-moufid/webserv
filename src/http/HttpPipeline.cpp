#include "http/HttpPipeline.hpp"	// buildResponse, buildResponseFromRaw, errorResponse, logRequest, resolve
#include "utils/MimeUtils.hpp"		// MimeUtils::mimeByPath
#include "utils/StringUtils.hpp"	// currentTime, toString, trim
#include "http/HttpStatusCodes.hpp"	// Code, ContentTooLarge, Created, Forbidden, InternalServerError and 7 more

#include <dirent.h>				// opendir, readdir, closedir
#include <fstream>				// ifstream
#include <iomanip>				// setw
#include <sstream>				// ostringstream
#include <sys/stat.h>			// stat, S_ISDIR
#include <iostream>				// cout, ios, left
#include <cstdlib>				// strtol
#include <unistd.h>				// stat

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
		if (name == "." || name == "..") continue;
		struct stat st;
		if (stat((path + "/" + name).c_str(), &st) != 0) continue;
		bool isDir = S_ISDIR(st.st_mode);
		std::string display = name + (isDir ? "/" : "");
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
	if (stat(filePath.c_str(), &st) != 0 || !std::ifstream(filePath.c_str()))
		return HttpPipeline::errorResponse(request, HttpStatus::Forbidden);

	HttpResponse res;
	res.setStatus(HttpStatus::OK);
	res.setContentType(MimeUtils::mimeByPath(filePath));
	res.setFile(filePath, st.st_size);
	return res;
}

HttpResponse staticResponse(const HttpRequest &request)
{
	const Route &route = *request.route;
	std::string root = route.root();
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

} // namespace

namespace HttpPipeline
{

void resolve(HttpRequest &request, const ListenEndpoints &endpoints, const Interface &iface)
{
	request.vhost = NULL;
	request.route = NULL;
	ListenEndpoints::const_iterator it = endpoints.find(iface);
	if (it == endpoints.end() || it->second.empty())
		return;
	if (!(request.vhost = resolveVhost(it->second, request.host())))
		return;
	request.route = resolveRoute(*request.vhost, request.uri().path);
}

HttpResponse handlePostRequest(const HttpRequest &)
{
	return HttpResponse::HttpResponseBuilder(HttpStatus::Created, "{\"status\":\"ok\"}", "application/json");
}

HttpResponse buildResponse(const HttpRequest &request)
{
	if (request.erroneous())
		return errorResponse(request, request.errorCode());

	if (!request.route)
		return errorResponse(request, HttpStatus::NotFound);

	if (request.contentLength() > request.route->maxBodySize())
		return errorResponse(request, HttpStatus::ContentTooLarge);

	if (request.route->redirected())
		return HttpResponse::HttpResponseRedirect(request.route->redirectCode(), request.route->redirectPage());

	if (request.hasCgi())
		return errorResponse(request, HttpStatus::InternalServerError);

	if (!request.route->upload().empty() && (request.method() == "POST" || request.method() == "PUT"))
		return handlePostRequest(request);

	if (request.method() == "GET")
		return staticResponse(request);

	return errorResponse(request, HttpStatus::MethodNotAllowed);
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

HttpResponse buildResponseFromRaw(const HttpRequest &request, const std::string &rawOutput)
{
	size_t headerEnd = rawOutput.find("\r\n\r\n"), step = 4;

	if (headerEnd == std::string::npos)
	{
		headerEnd	= rawOutput.find("\n\n");
		step		= 2;
	}
	if (headerEnd == std::string::npos)
		return errorResponse(request, HttpStatus::InternalServerError);

	std::istringstream ss(rawOutput.substr(0, headerEnd));
	std::string body = rawOutput.substr(headerEnd + step);

	HttpResponse response;
	std::string line;

	while (std::getline(ss, line))
	{
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		size_t colon = line.find(':');
		if (colon == std::string::npos) continue;
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
		else
			response.setHeader(name, value);
	}

	if (HttpStatus::isErrorCode(response.statusCode()) && body.empty())
		return errorResponse(request, response.statusCode());

	response.setBody(body);
	return response;
}

}
