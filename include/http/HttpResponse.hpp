#pragma once

#include "http/HttpStatusCodes.hpp"
#include <string>
#include <map>

class HttpResponse {
    public:
       HttpResponse(void);

       void setStatus(HttpStatus::Code status);
       void setHeader(const std::string &name, const std::string &value);
       void setContentType(std::string &type);
       void setBody(const std::string &body);

       // serialize: build the complete response.
       std::string serialize(void) const;

       static HttpResponse HttpResponseError(HttpStatus::Code status, const std::string &pagePath);
       static HttpResponse HttpResponseRedirect(HttpStatus::Code status, const std::string& location);


    private:
        HttpStatus::Code                        status_;
        std::map<std::string, std::string>      headers_;
        std::string                             body_;
}