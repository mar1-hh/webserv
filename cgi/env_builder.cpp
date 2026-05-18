#include "cgi.hpp"

std::vector<std::string> env_buider(HttpRequest& req, std::string script_name, 
    std::string server_name, std::string server_port, std::string client_ip, std::string path_info)
{
    std::vector<std::string> cgi_env;
    std::map<std::string, std::string> headers_map;
    std::map<std::string, std::string>::iterator it;

    cgi_env.push_back("REQUEST_METHOD=" + req.getMethod());
    cgi_env.push_back("PATH_INFO=" + path_info);
    cgi_env.push_back("QUERY_STRING=" + req.getQuery());
    cgi_env.push_back("CONTENT_TYPE=" + req.getHeader("content-type"));
    cgi_env.push_back("CONTENT_LENGTH=" + req.getHeader("content-length"));
    cgi_env.push_back("SCRIPT_NAME=" + script_name);
    cgi_env.push_back("REQUEST_URI=" + req.getUri());
    cgi_env.push_back("SERVER_PROTOCOL=" + req.getHttpVersion());
    cgi_env.push_back("GATEWAY_INTERFACE=CGI/1.1");
    cgi_env.push_back("SERVER_NAME=" + server_name);
    cgi_env.push_back("SERVER_PORT=" + server_port);
    cgi_env.push_back("REMOTE_ADDR=" + client_ip);

    headers_map = req.getHeaders();
    for (it = headers_map.begin(); it != headers_map.end(); it++)
    {
        std::string key = it->first;
        std::string value = it->second;

        if (key == "CONTENT_TYPE" || key == "CONTENT_LENGTH")
            continue ;
        for (int i = 0; i < key.length(); i++)
        {
            if (key[i] == '-')
                key[i] = '_';
            else
                key[i] = std::toupper(key[i]);
        }
        cgi_env.push_back("HTTP_" + key + "=" + value);
    }
    return (cgi_env);
}
