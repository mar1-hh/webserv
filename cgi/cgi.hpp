#pragma once

#include "../request-responce/HttpRequest.hpp"
#include "../server_core/ConfigFileParser/parser.hpp"
#include "unistd.h"


struct cgi_result {
    std::string raw_output;
    int exit_status;
};

std::vector<std::string> env_buider(HttpRequest& req, std::string script_name, 
    std::string server_name, std::string server_port, std::string client_ip, std::string path_info);

cgi_result exeute_cgi(HttpRequest& req, Server& ser, Location& loc, std::string client_ip);

