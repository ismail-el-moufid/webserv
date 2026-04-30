#pragma once

#include "../config/Route.hpp"
#include "../config/VirtualHost.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "../core/Pipe.hpp"
#include <vector>
#include <unistd.h>
#include "utils/NetworkUtils.hpp"
#include <iostream>

class CGIProcess
{
public:
    CGIProcess();

    bool running() const;

    Pipe        stdin_;
    Pipe        stdout_;
    pid_t       pid_;
    time_t      started_;
    std::string output_buffer_;
    ssize_t      bytes_sent_;

private:
    CGIProcess(const CGIProcess &);
    CGIProcess &operator=(const CGIProcess &);
    ~CGIProcess();
};

class CGIHandler
{
public:
    static void start(CGIProcess &cgi, const HttpRequest &request, const Interface &iface);
    static void writeStdin(CGIProcess &cgi, const HttpRequest &request);
    static void readStdout(CGIProcess &cgi);
    static void finish(CGIProcess &cgi, HttpResponse &response);
    static void kill_Process(CGIProcess &cgi, HttpResponse &response);

private:
    static std::vector<std::string> buildEnv(const HttpRequest &request, const Interface &iface);
    static std::vector<std::string> buildArg(const std::string &filename, const HttpRequest &request);
    static char                   **toCharArray(const std::vector<std::string> &vec);
    static std::string              resolveScript(const HttpRequest &request, std::vector<std::string> &env);
};