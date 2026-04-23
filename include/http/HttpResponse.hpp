#pragma once

#include "http/HttpStatusCodes.hpp"
#include <string>
#include <map>

class HttpResponse {
    public:
       HttpResponse(void);

       void setStatus(HTTPStatus::Code status);
       void setHeader(std::string &name, std::string &value);
       void setBody(std::string &body);


    private:
        HTTPStatus::Code                        status_;
        std::map<std::string, std::string>      headers_;
        std::string                             body_;
}