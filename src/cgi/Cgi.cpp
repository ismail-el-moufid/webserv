#include "../include/cgi/Cgi.hpp"

std::string    CGIHandler::resolveScript(const HttpRequest &request, std::vector<std::string> &env)
{
    std::string uri = request.uri();
    size_t qm_pos = uri.find('?');
    std::string path_part;
    std::string filename;
    
    if (qm_pos != std::string::npos) {
        path_part = uri.substr(0, qm_pos); 
        env.push_back("QUERY_STRING=" + uri.substr(qm_pos + 1));
    } else {
        path_part = uri;
        env.push_back("QUERY_STRING=");
    }
    std::string script_name = path_part; 
    size_t dot_pos = path_part.find('.'); 
    if (dot_pos != std::string::npos) {
        size_t path_info_slash = path_part.find('/', dot_pos);

        if (path_info_slash != std::string::npos) {
            script_name = path_part.substr(0, path_info_slash);
            std::string path_info = path_part.substr(path_info_slash); 
            // env.push_back("PATH_INFO=" + path_info);
            // env.push_back("PATH_TRANSLATED=" + iface.root() + path_info);
        }
    }
    // filename = route.root() + script_name;
    env.push_back("SCRIPT_NAME=" + script_name);
    env.push_back("SCRIPT_FILENAME=" + filename);
    return filename;
}
std::vector<std::string> CGIHandler::buildEnv(const HttpRequest &request, const Interface &iface)
{
    std::vector<std::string> env;
     env.push_back("REQUEST_METHOD=" + request.method());
    env.push_back("SERVER_PROTOCOL=" + request.version());
    std::string uri = request.uri();
    const std::map<std::string, std::string> headers = request.headers();
    std::map<std::string,std::string>::const_iterator i;
    for(i = headers.begin(); i != headers.end(); i++)
    {
        std::string key = i->first;
        std::string value = i->second;
        for(size_t idx = 0; idx < key.length(); idx++)
        {
            key[idx] = toupper(key[idx]);
            if(key[idx] == '-')
                key[idx] = '_';
        }
        if(key == "CONTENT_LENGTH" || key  == "CONTENT_TYPE")
            env.push_back(key + "=" + value);
        else
            env.push_back("HTTP_" + key + "=" + value);
    }
    env.push_back("SERVER_SOFTWARE=WebServ/1.0");
    env.push_back("GATEWAY_INTERFACE=CGI/1.1");
    env.push_back("REDIRECT_STATUS=200");
    // env.push_back("SERVER_NAME=" + vhost.name());
    // env.push_back("SERVER_PORT=" + vhost.binds()[0].second);
    // env.push_back("REMOTE_ADDR="); // we need client ip for this;
    env.push_back("REQUEST_URI=" + request.uri());
}
char   **CGIHandler::toCharArray(const std::vector<std::string> &vec)
 {
    char    **arr = new char*[vec.size() + 1];
    int i = 0;
    for(std::vector<std::string>::const_iterator it = vec.begin(); it != vec.end(); ++it)
        arr[i++] = const_cast<char *>(it->c_str());
    arr[i++]= (NULL);
    return(arr);
}
std::vector<std::string> CGIHandler::buildArg(const std::string &filename, const HttpRequest &request)
{
    size_t dot_pos = filename.rfind('.');
    std::vector<std::string> arg;
    std::string extention = filename.substr(dot_pos);
    // std::map<std::string, std::string>::const_iterator it = route.CGIs().find(extention);
    // arg.push_back(it->second);
    // arg.push_back(filename);
    return(arg);
}

void CGIHandler::start(CGIProcess &cgi, const HttpRequest &request, const Interface &iface)
{
    char **argumnets;
    char **envm;
    std::string filename;
    std::vector<std::string> env = buildEnv(request, iface);
    filename = resolveScript(request, env);
    std::vector<std::string> arg = buildArg(filename,request);
    argumnets = toCharArray(arg);
    envm = toCharArray(env);
    
}

void CGIHandler::writeStdin(CGIProcess &cgi, const HttpRequest &request)
{
    ssize_t written;

    if(cgi.bytes_sent_ >= request.contentLength())
    {
        cgi.stdin_.closeWrite();
        //change stat of client
        return;
    }
    ssize_t remaining = request.contentLength() - cgi.bytes_sent_;
    written = write(cgi.stdin_.getWriteFd(), request.body().c_str() + cgi.bytes_sent_ , remaining);
    if(written > 0)
        cgi.bytes_sent_ += written;
    else if(written == -1)
        return ;//hundle eror
}

void CGIHandler::readStdout(CGIProcess &cgi)
{
    ssize_t bytes;
    char buffer[4096];
    bytes =read(cgi.stdout_.getReadFd(), buffer, 4096);
    if(bytes > 0)
        cgi.output_buffer_.append(buffer, bytes);
    else if(bytes == 0)
    {
        cgi.stdout_.closeRead();
       //change state
    }
    else
        return; //hundle errors
}
// void CGI::execute_cgi(const HttpRequest &request)
// {
//     Pipe stdin_pipe;
//     Pipe stdout_pipe;
//     int status;
//     ssize_t n;
//     char **arg = getarg();
//     char **env = getenv();
//     pid_t pid = fork();
//     switch (pid)
//     {
//     case -1:
//         this->status = 500;
//         return;
//     case 0:
//         dup2(stdin_pipe.getReadFd(), 0);
//         dup2(stdout_pipe.getWriteFd(), 1);
//         execve(arg[0],arg, env);
//         perror("execve");
//         exit(1);
//     default:
//         stdin_pipe.closeRead();
//         stdout_pipe.closeWrite();
//         if(request.method() == "POST")
//             write(stdin_pipe.getWriteFd(), request.body().c_str(), request.contentLength());
//         stdin_pipe.closeWrite();
//         char buffer[1000];
//         while ((n =read(stdout_pipe.getReadFd(), buffer, 1000)) > 0)
//                 _respond.append(buffer,n);
//         stdout_pipe.closeRead();
//         waitpid(pid, &status, 0);
//         if(!WIFEXITED(status) || WIFSIGNALED(status) != 0)
//             this->status = 500;
//         else
//             this->status = 200;
//     }
// }
// void cgi(const HttpRequest &request, const VirtualHost &vhost, const Route &route)
// {
//     CGI cgi;
//     std::string filename;
//     filename = cgi.setenv(request, vhost, route);
//     cgi.setarg(filename, route);
//     cgi.execute_cgi(request);
// }

// std::string split_uri(std::vector<std::string> &env, const HttpRequest &request, const Route &route)
// {
//     std::string uri = request.uri();
//     size_t qm_pos = uri.find('?');
//     std::string path_part;
//     std::string filename;
    
//     if (qm_pos != std::string::npos) {
//         path_part = uri.substr(0, qm_pos); 
//         env.push_back("QUERY_STRING=" + uri.substr(qm_pos + 1));
//     } else {
//         path_part = uri;
//         env.push_back("QUERY_STRING=");
//     }
//     std::string script_name = path_part; 
//     size_t dot_pos = path_part.find('.'); 
//     if (dot_pos != std::string::npos) {
//         size_t path_info_slash = path_part.find('/', dot_pos);

//         if (path_info_slash != std::string::npos) {
//             script_name = path_part.substr(0, path_info_slash);
//             std::string path_info = path_part.substr(path_info_slash);
            
//             env.push_back("PATH_INFO=" + path_info);
//             env.push_back("PATH_TRANSLATED=" + route.root() + path_info);
//         }
//     }
//     filename = route.root() + script_name;
//     env.push_back("SCRIPT_NAME=" + script_name);
//     env.push_back("SCRIPT_FILENAME=" + filename);
//     return filename;
// }
// std::string CGI::setenv(const HttpRequest &request, const VirtualHost &vhost, const Route &route)
// {
//     std::string filename;

//     _env.push_back("REQUEST_METHOD=" + request.method());
//     _env.push_back("SERVER_PROTOCOL=" + request.version());
//     std::string uri = request.uri();
//     filename = split_uri(_env, request, route);
//     const std::map<std::string, std::string> headers = request.headers();
//     std::map<std::string,std::string>::const_iterator i;
//     for(i = headers.begin(); i != headers.end(); i++)
//     {
//         std::string key = i->first;
//         std::string value = i->second;
//         for(size_t idx = 0; idx < key.length(); idx++)
//         {
//             key[idx] = toupper(key[idx]);
//             if(key[idx] == '-')
//                 key[idx] = '_';
//         }
//         if(key == "CONTENT_LENGTH" || key  == "CONTENT_TYPE")
//             _env.push_back(key + "=" + value);
//         else
//             _env.push_back("HTTP_" + key + "=" + value);
//     }
//     _env.push_back("SERVER_SOFTWARE=WebServ/1.0");
//     _env.push_back("GATEWAY_INTERFACE=CGI/1.1");
//     _env.push_back("REDIRECT_STATUS=200");
//     _env.push_back("SERVER_NAME=" + vhost.name());
//     _env.push_back("SERVER_PORT=" + vhost.binds()[0].second);
//     _env.push_back("REMOTE_ADDR="); // we need client ip for this;
//     _env.push_back("REQUEST_URI=" + request.uri());
//     return filename;
// }

// void CGI::setarg(std::string filename, const Route &route)
// {
//     size_t dot_pos = filename.rfind('.');

//     std::string extention = filename.substr(dot_pos);
//     std::map<std::string, std::string>::const_iterator it = route.CGIs().find(extention);
//     _arg.push_back(it->second);
//     _arg.push_back(filename);
// }

// char **CGI::getarg()
// {
//     for(std::vector<std::string>::iterator it = _arg.begin(); it != _arg.end(); ++it)
//         _argv.push_back(it->c_str());
//     _argv.push_back(NULL);
//     return(const_cast<char**>(_argv.data()));
// }

// char **CGI::getenv()
// {
//     for(std::vector<std::string>::const_iterator it = _env.begin(); it != _env.end(); ++it)
//         _envm.push_back(it->c_str());
//     _envm.push_back(NULL);
//     return(const_cast<char**>(_envm.data()));
// }

// std::vector<std::string> CGIHandler::buildEnv(const HttpRequest &request, const Interface &iface)
// {
//    std::vector<std::string> env;

//     td::string filename;

    
//     env.push_back("REQUEST_METHOD=" + request.method());
//     env.push_back("SERVER_PROTOCOL=" + request.version());
//     std::string uri = request.uri();
//     filename = split_uri(env, request, route);
//     const std::map<std::string, std::string> headers = request.headers();
//     std::map<std::string,std::string>::const_iterator i;
//     for(i = headers.begin(); i != headers.end(); i++)
//     {
//         std::string key = i->first;
//         std::string value = i->second;
//         for(size_t idx = 0; idx < key.length(); idx++)
//         {
//             key[idx] = toupper(key[idx]);
//             if(key[idx] == '-')
//                 key[idx] = '_';
//         }
//         if(key == "CONTENT_LENGTH" || key  == "CONTENT_TYPE")
//             env.push_back(key + "=" + value);
//         else
//             env.push_back("HTTP_" + key + "=" + value);
//     }
//     env.push_back("SERVER_SOFTWARE=WebServ/1.0");
//     env.push_back("GATEWAY_INTERFACE=CGI/1.1");
//     env.push_back("REDIRECT_STATUS=200");
//     env.push_back("SERVER_NAME=" + vhost.name());
//     env.push_back("SERVER_PORT=" + vhost.binds()[0].second);
//     env.push_back("REMOTE_ADDR="); // we need client ip for this;
//     env.push_back("REQUEST_URI=" + request.uri());
//     return filename;
// }
