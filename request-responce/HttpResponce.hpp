#ifndef HTTPRESPONCE_HPP
#define HTTPRESPONCE_HPP
#include "HttpRequest.hpp"
#include "../server_core/ConfigFileParser/parser.hpp"
#include <sstream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include "../cgi/cgi.hpp"

class Server_core;
class HttpResponce {
private:
    Location _location;
    std::string real_path;
    HttpRequest &req;
    Server &server;
    std::string client_ip;

    std::string fileContent;
    std::string contentType;
    int fileSize;

    size_t status_code;
    std::string status_message;
    std::string responce;
    std::string cgi_headers;

    bool cgi_flag;

public:
    HttpResponce(HttpRequest &request, Server &serv, std::string client_ip);
    bool is_cgi(std::string path);
    void proccess();
    void HandleGet();
    void HandlePost();
    void HandleDelete();
    void craftResponce();
    void HandleCgi();

    void setFileType();
    void readFile();
    void handleDir();

    bool validateLocation();
    bool validateMethod();

    void handleindex();
    void handleListing();

    std::string getResponce() const;
};
#endif