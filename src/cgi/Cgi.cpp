#include "cgi/Cgi.hpp"
#include "core/Client.hpp"
#include "core/IOReactor.hpp"
#include "utils/StringUtils.hpp"
#include <signal.h>
#include <cstdlib>

std::string    CGIHandler::resolveScript(const HttpRequest &request, std::vector<std::string> &env, const Route &route)
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
            env.push_back("PATH_INFO=" + path_info);
            env.push_back("PATH_TRANSLATED=" + route.root() + path_info);
        }
    }
    filename = route.root() + script_name;
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
            key[idx] = std::toupper(key[idx]);
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
    // env.push_back("SERVER_NAME=" + );
    std::string port,ip;
    NetworkUtils::extractIPPort(iface, ip, port);
    env.push_back("SERVER_PORT=" + port);
    env.push_back("REMOTE_ADDR=" + ip); 
    env.push_back("REQUEST_URI=" + request.uri());
    return env;
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
std::vector<std::string> CGIHandler::buildArg(const std::string &filename, const Route &route)
{
    size_t dot_pos = filename.rfind('.');
    std::vector<std::string> arg;
    std::string extention = filename.substr(dot_pos);
    std::map<std::string, std::string>::const_iterator it = route.cgis().find(extention);
    arg.push_back(it->second);
    arg.push_back(filename);
    return(arg);
}

void CGIHandler::start(Client &client)
{
    char **argumnets;
    char **envm;
    pid_t pid;

    CGIProcess &cgi = *(client.cgi);
    std::string filename;
    std::vector<std::string> environ = buildEnv(client.request, client.iface);
    std::vector<const char *> env = StringUtils::toNullTerminatedCStrings(environ);
    filename = resolveScript(client.request, environ, *client.request.route);
    std::vector<const char *> arg = StringUtils::toNullTerminatedCStrings(buildArg(filename, *client.request.route));
    argumnets = const_cast<char**>(&arg[0]);
    envm = const_cast<char**>(&env[0]);
    cgi.started_ = time(NULL);
    switch (pid = fork())
    {
    case -1:
        delete  client.cgi;
        client.cgi = NULL;
        client.response.setStatus(HttpStatus::InternalServerError);
        return ;
    case 0:
        dup2(cgi.stdin_.readFd(), 0);
        dup2(cgi.stdout_.writeFd(), 1);
        execve(argumnets[0], argumnets, envm);
        exit(1);
        break ;
    default:
        cgi.stdin_.closeRead();
        cgi.stdout_.closeWrite();
        cgi.pid_ = pid;
    }
}

void CGIHandler::finish(Client &client)
{
    int status;
    CGIProcess &cgi = *client.cgi;
    HttpResponse &response = client.response;
    waitpid(cgi.pid_, &status, 0);
    if(WIFEXITED(status) || WIFSIGNALED(status) == 0)
    {   response.setStatus(HttpStatus::OK);
        response.setBody(cgi.output_buffer_);
    }
    else  
        response.setStatus(HttpStatus::InternalServerError);
            //add body function
}

void    CGIHandler::killProcess(Client &client)
{
    int status;
    HttpResponse &response = client.response;
    CGIProcess &cgi = *client.cgi;

    kill(cgi.pid_, SIGKILL);
    waitpid(cgi.pid_, &status, 0);
    response.setStatus(HttpStatus::InternalServerError);
    //add body function
}


bool CGIHandler::writeStdin(Client &client)
{
    ssize_t written;
    HttpResponse &response = client.response;
    CGIProcess &cgi = *client.cgi;
    const HttpRequest &request = client.request;

    if(cgi.bytes_sent_ >= request.contentLength())
    {
        cgi.stdin_.closeWrite();
        return true;
    }
    size_t remaining = request.contentLength() - cgi.bytes_sent_;
    written = write(cgi.stdin_.writeFd(), request.body().c_str() + cgi.bytes_sent_ , remaining);
    if(written > 0)
        cgi.bytes_sent_ += written;
    else if(written == -1)
        response.setStatus(HttpStatus::InternalServerError);
        //body funct;
    return false;
}

bool CGIHandler::readStdout(Client &client)
{
    ssize_t bytes;
    CGIProcess &cgi = *client.cgi;
    char buffer[4096];
    HttpResponse &response = client.response;

    bytes = read(cgi.stdout_.readFd(), buffer, 4096);
    if(bytes > 0)
        cgi.output_buffer_.append(buffer, bytes);
    else if(bytes == 0)
    {    
        cgi.stdout_.closeRead();
        return true;    
    }
    else
       response.setStatus(HttpStatus::InternalServerError);
    //body funct;
    return false;                                                                                               
}
CGIProcess::CGIProcess(IOReactor &reactor, Client &client) : IPollable(reactor), pid_(-1), started_(0), bytes_sent_(0), client_(&client) {}

CGIProcess::~CGIProcess() { reactor_.remove(*this); }

void CGIProcess::onWrite()
{
    if (CGIHandler::writeStdin(*client_))
        reactor_.mod(*this, POLLIN);
}

void CGIProcess::onRead()
{
    if (CGIHandler::readStdout(*client_))
    {
        CGIHandler::finish(*client_);
        reactor_.remove(*this);
        client_->cgi = NULL;
        delete this;
    }
}

int CGIProcess::readFd()    const { return stdout_.readFd(); }
int CGIProcess::writeFd()    const { return stdin_.writeFd(); }
bool CGIProcess::running()    const { return pid_ != -1; }