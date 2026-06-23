#ifndef PARSER_HPP
#define PARSER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <map>

struct Location
{
    std::string path;
    std::string root;
    std::vector<std::string> methods;
    std::string redirection;
    bool directory_listing;
    std::string default_file;
    bool upload_enabled;
    std::string upload_path;
    std::map<std::string, std::string> cgi;

    Location();
};

struct Server
{
    std::string host;
    int port;
    std::string root;
    std::vector<Location> locations;
    std::map<int, std::string> error_pages;
    size_t max_body_size;

    Server();
};

bool parser(std::vector<Server> &servers, const std::string &path);

#endif
