#ifndef HTTPRESPONCE_HPP
#define HTTPRESPONCE_HPP
#include "HttpRequest.hpp"
#include "../server_core/ConfigFileParser/parser.hpp"

class Server_core;
class HttpResponce {
private:
    Location _location;
    std::string real_path;
    HttpRequest &req;
    Server &server;

    size_t status_code;
    std::string status_message;
    std::string responce;

public:
    HttpResponce(HttpRequest &request, Server &serv);
    void proccess();
    void craftResponce();

    bool validateLocation();
    bool validateMethod();


    std::string getResponce() const;
};
#endif