#pragma once

#include "../config/Route.hpp"
#include "../config/VirtualHost.hpp"
#include "../http/HttpRequest.hpp"
#include "../core/Pipe.hpp"
#include <vector>
#include <unistd.h>


class CGI
{
    private:
    std::vector<std::string> _env;
    std::vector<std::string> _arg;
    std::vector<const char*>  _argv;
    std::vector<const char*>  _envm;
    std::string _respond;

    int status;
    public:
        CGI();
        ~CGI();
        std::string setenv(const HttpRequest &request, const VirtualHost &vhost, const Route &route);
        void setarg(std::string filename, const Route &route);
        char **getenv();
        char **getarg();
        void execute_cgi(const HttpRequest &request);
};



void cgi(const HttpRequest &request, const VirtualHost &vhost, const Route &route);