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
    ores << "HTTP/1.1" << std::static_cast<int>(status_) << " " << HttpStatusCodes::reasonPhrase(status_) << "\r\n";

    // write headers, but need more checks ...
    for (std::map<std::string, std::string>::const_iterator it = headers_.begin(); it != headers_end(); ++it) {
        ores << it->first << ": " << it->second << "\r\n";
    }

    // Content-Length: we need to decide where to set it to avoid the overhead of find, 
    // for now let's just pretend that it not set.
    if (headers_.find("Content-Length") == _headers_end()) {
        ores << "Content-Length: " << body_.size() << "\r\n";
    }

    //end of headers;
    ores << "\r\n";

    // write body
    ores << body_;
    return ores.str();
}