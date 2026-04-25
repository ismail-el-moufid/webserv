#include "http/HttpResponse.hpp"
#include <sstream>                  //ostringstream

HttpResponse::HttpResponse(void): status_(HttpStatusCodes::OK){}

HttpResponse::HTTPStatus(HTTPStatus::Code status) {
    status_ = status;
}

HttpResponse::setHeader(std::string &name, std::string &value) {
    headers_[name] = value;
}

HttpResponse::setBody(std::string &body) {
    body_ = body;
}

std::string HttpResponse::serialize(void) const {
    std::ostringstream ores;

    // 1.1 or 1.0? it depend, let's just stick with 1.1 for now.
    ores << "HTTP/1.1 " << std::static_cast<int>(status_) << " " << HttpStatusCodes::reasonPhrase(status_) << "\r\n";

    // write headers, but need more checks ...
    for (std::map<std::string, std::string>::const_iterator it = headers_.begin(); it != headers_.end(); ++it) {
        ores << it->first << ": " << it->second << "\r\n";
    }

    // Content-Length: we need to decide where to set it to avoid the overhead of find, 
    // for now let's just pretend that it not set.
    if (headers_.find("Content-Length") == headers_.end()) {
        ores << "Content-Length: " << body_.size() << "\r\n";
    }

    // Connection: default close, but it depend on the requested client;
    if (headers_.find("Connection") == headres_.end()) {
        ores << "Connection: close" << "\r\n";
    }

    //end of headers;
    ores << "\r\n";

    // write body
    ores << body_;
    return ores.str();
}

void HttpResponse::setContentType(std::string& type) {
    if (type.empty()) {
        // or text/html? 
        type = "text/plain";
    }
    headers_["Content-Type"] = type;
}

static std::string readFile(const char* filename) {
    std::ifstream file(filename);
    if (!file) return "";
    
    std::ostringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

static std::string defaultErrorBody(HTTPStatus::Code status) {
    std::ostringstream out;

    out << "<!doctype html> <html lang=\"en-us\"><head><title>" << static_cast<int>(status) << " - "
        << HTTPStatus::reasonPhrase(status) << "</title></head>"
        << "<body><h1>" << static_cast<int>(status) << " " << HTTPStatus::reasonPhrase(status) << "</h1></body></html>"
     
    return out.str();
}

HttpResponse HttpResponse::HttpResponseError(HTTPStatus::Code status, const std::string &pagePath) {
    HttpResponse res;

    std::string body = readFile(pagePath.c_str());
    if (body.empty()) {
        // no file locate at pagePath
        // serve default error page.
        body = defaultErrorBody(status);
    }
    res.setStatus(status);
    res.setContentType("text/html");
    res.setBody(body);
    
    return res;
}

HttpResponse HttpResponse::HttpResponseRedirect(HttpStatus::Code status, std::string &location) {
    HttpResponse res;

    if (location.empty()) {
        location = "/";
    }
    res.setStatus(status);
    res.setHeader("Location", location);
    res.setContentType("text/html");
    
    std::ostringstream body;
    body << "<!doctype html> <html lang=\"en-us\"><head><title>Redirect ...</title></head>"
         << "<body> redirecting to <a href=\"" << location << "\"></a></body></html>";
    res.setBody(body.str());
    return res;
}