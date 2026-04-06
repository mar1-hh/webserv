#ifndef PARSER_HPP
#define PARSER_HPP

#include <iostream>
#include <vector>


typedef struct Location {
    std::string path;
    std::string root;
    std::vector<std::string> methods; 
    std::string redirection;           
    bool directory_listing;         
    std::string default_file;    
    bool upload_enabled;               
    std::string upload_path;           
    std::string cgi_extension;        
    std::string cgi_path;              
} Location;

typedef struct Server {
    std::string interface_ip;         
    int port;
    std::string root;
    std::vector<Location> locations;
    std::map<int, std::string> error_pages;  
    size_t max_body_size;     
} Server;

#endif