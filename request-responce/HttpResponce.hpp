#ifndef HTTPRESPONCE_HPP
#define HTTPRESPONCE_HPP

#include "HttpRequest.hpp"
#include "Session.hpp"
#include "../server_core/ConfigFileParser/parser.hpp"
#include <string>
#include <vector>

class HttpResponce
{
private:
    HttpRequest &req;
    Server &server;
    SessionManager &sessions;

    Location _location;
    bool _located;
    std::string real_path;

    std::string fileContent;
    std::string contentType;
    int status_code;
    std::vector<std::string> extra_headers;
    std::string responce;

    bool _is_cgi;
    std::string _cgi_interp;
    std::string _cgi_script;
    std::string _cgi_script_name;
    std::string _cgi_path_info;

    bool matchLocation();
    bool methodAllowed() const;
    std::string allowedMethods() const;
    bool detectCgi();

    void handleGet();
    void handlePost();
    void handleDelete();
    void handleDir();
    void serveFile(const std::string &fsPath);
    void listDirectory();
    void handleSession();

    void build();
    void buildErrorBody();

public:
    HttpResponce(HttpRequest &request, Server &serv, SessionManager &sm);

    void process();

    bool isCgi() const;
    const std::string &cgiInterpreter() const;
    const std::string &cgiScript() const;
    const std::string &cgiScriptName() const;
    const std::string &cgiPathInfo() const;

    std::string getResponce() const;
};

#endif
