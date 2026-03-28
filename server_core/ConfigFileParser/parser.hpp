#ifndef PARSER_HPP
#define PARSER_HPP

#include <iostream>
#include <vector>

typedef struct Location {
    std::string path;
    std::string root;
    std::vector<std::string> methods;
} Location;

typedef struct Server {
    int port;
    std::string root;
    std::vector<Location> locations;
} Server;

#endif