#include "http/HttpResponse.hpp"

HttpResponse::HttpResponse(void): status_(HttpStatusCodes::OK){}

HttpResponse::HTTPStatus(HTTPStatus::Code status) {
    status_ = status;
}

HttpResponse::setHeader(std::string &name, std::string &value) {
    headers_[name] = value;
}

HTTPStatus::setBody(std::string &body) {
    body_ = body;
}