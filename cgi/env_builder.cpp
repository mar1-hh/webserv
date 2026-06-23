#include "cgi.hpp"
#include <sstream>
#include <cctype>

std::vector<std::string> build_cgi_env(HttpRequest &req,
                                       const std::string &script_name,
                                       const std::string &script_filename,
                                       const std::string &path_info,
                                       const std::string &server_name,
                                       const std::string &server_port,
                                       const std::string &client_ip)
{
    std::vector<std::string> env;
    std::string protocol = req.getHttpVersion().empty() ? "HTTP/1.1" : req.getHttpVersion();

    env.push_back("GATEWAY_INTERFACE=CGI/1.1");
    env.push_back("SERVER_SOFTWARE=webserv/1.0");
    env.push_back("SERVER_PROTOCOL=" + protocol);
    env.push_back("REDIRECT_STATUS=200");
    env.push_back("REQUEST_METHOD=" + req.getMethod());
    env.push_back("REQUEST_URI=" + req.getUri());
    env.push_back("QUERY_STRING=" + req.getQuery());
    env.push_back("SCRIPT_NAME=" + script_name);
    env.push_back("SCRIPT_FILENAME=" + script_filename);
    env.push_back("PATH_INFO=" + path_info);
    env.push_back("SERVER_NAME=" + server_name);
    env.push_back("SERVER_PORT=" + server_port);
    env.push_back("REMOTE_ADDR=" + client_ip);

    std::string ct = req.getHeader("content-type");
    if (!ct.empty())
        env.push_back("CONTENT_TYPE=" + ct);

    std::ostringstream len;
    len << req.getBody().size();
    env.push_back("CONTENT_LENGTH=" + len.str());

    std::map<std::string, std::string> headers = req.getHeaders();
    std::map<std::string, std::string>::iterator it;
    for (it = headers.begin(); it != headers.end(); ++it)
    {
        if (it->first == "content-type" || it->first == "content-length")
            continue;
        std::string key;
        for (size_t i = 0; i < it->first.size(); i++)
        {
            char c = it->first[i];
            key += (c == '-') ? '_' : static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        env.push_back("HTTP_" + key + "=" + it->second);
    }
    return env;
}
