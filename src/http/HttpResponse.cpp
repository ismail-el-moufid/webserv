#include "http/HttpResponse.hpp"

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
    std::osstringstream ores;

    // 1.1 or 1.0? it depend, let's just stick with 1.1 for now.
    ores << "HTTP/1.1" << std::static_cast<int>(status_) << " " << HttpStatusCodes::reasonPhrase(status_) << "\r\n";

    // write headers, but need more checks ...
    for (std::map<std::string, std::string>::const_iterator it = headers_.begin(); it != headers_end(); ++it) {
        ores << it->first << ": " << it->second << "\r\n";
    }

    //end of headers;
    orse << "\r\n";

    // write body
    ores << body_;
    return ores.str();
}