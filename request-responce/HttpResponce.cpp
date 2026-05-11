#include "HttpResponce.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sstream>

void display_response(const HttpResponce &r)
{
    std::cout << "\n";
    std::cout << "┌─────────────────────────────────────────┐" << std::endl;
    std::cout << "│          HTTP RESPONSE BUILT             │" << std::endl;
    std::cout << "└─────────────────────────────────────────┘" << std::endl;

    std::cout << "responce :" << std::endl;
    std::cout << r.getResponce() << std::endl;
    std::cout << std::endl;
    std::cout << std::endl;
    std::cout << std::endl;
    std::cout << std::endl;
    std::cout << std::endl;
}

HttpResponce::HttpResponce(HttpRequest &request, Server &serv): req(request), server(serv){
    status_code = 200;
    proccess();
    craftResponce();
    display_response(*this);
}
bool HttpResponce::validateLocation(){
    std::vector<Location> locations = server.locations;
    std::string location = req.getPath();
    size_t best_length =  0;
    bool found = false;
    
    std::vector<Location>::iterator it = locations.begin();
    while (it != locations.end()){
        
        if (location.find((*it).path) == 0)
        {
            std::cout << "got path " << (*it).path << std::endl;
            if ((*it).path.length() > best_length){
                best_length = (*it).path.length();
                _location = *it;
            }
            found = true;
        }
        it++;
    }
    if (found)
    {
        real_path = location.substr(best_length);
        if (_location.root.length() == 0)
            real_path = server.root +"/"+ real_path;
        else
            real_path = _location.root + "/" + real_path;

        if (req.getMethod() == "GET"){
            size_t fileFd = open(real_path.c_str(), 0);
            if (fileFd == -1)
                found = false;
            else
            {
                close(fileFd);
            }
    }
    }
    return found;
}

bool HttpResponce::validateMethod(){
    std::vector<std::string>::iterator it = _location.methods.begin();
    if (it == _location.methods.end())
    {
        status_code = 301;
        status_message = "301 Moved Permanently";
        return true;
    }
    while (it != _location.methods.end())
    {
        if (req.getMethod() == *it)
            return true;
        it++;
    }
    return false;
}

void HttpResponce::proccess(){
    if (!validateLocation())
    {
        status_code = 404;
        status_message = "404 Not Found";
        return;
    }
    if (!validateMethod())
    {
        status_code = 405;
        status_message = "405 Method Not Allowed";
        return;
    }
    if (req.getMethod() == "POST" && atoi(req.getHeader("Content-length").c_str()) >server.max_body_size)
    {
        status_code = 413;
        status_message = "413 Content Too Large";
        return;
    }
    status_message = "200 OK";
}

//getter 
std::string HttpResponce::getResponce() const{
    return responce;
}
std::string intToString(int n)
{
    std::ostringstream oss;
    oss << n;
    return oss.str();
}

void HttpResponce::craftResponce(){
    char buff[10001] = {0};
    std::string lenght;
    if (status_code != 200)
    {
        real_path = server.root +"/" + server.error_pages[status_code];
        std::cout << "error path is " << real_path << std::endl;
    }
    // handle redirection
    if (status_code == 301)
    {
        status_message += "\r\nLocation: " + _location.redirection;
    }
    if (req.getMethod() == "GET")
    {
        size_t fileFd = open(real_path.c_str(), 0);
        size_t fileSize = read(fileFd, buff, 10000);
        if (fileSize == -1)
            fileSize = 0;
        lenght = intToString(fileSize);
    }
    responce = "HTTP/1.1 " + status_message + "\r\nContent-Type: text/html\r\nContent-Length: "+ lenght + "\r\nConnection: keep-alive\r\n\r\n";
    responce += buff;
}