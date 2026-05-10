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
    size_t best_length;
    bool found;
    
    std::vector<Location>::iterator it = locations.begin();
    while (it != locations.end()){
        
        std::cout << "debuging root " <<server.root  << " " << (*it).path<< std::endl;
        if (location.find((*it).path) == 0)
        {
            std::cout << "got path " << (*it).path << std::endl;
            best_length = (*it).path.length();
            _location = *it;
            found = true;
        }
        it++;
    }
    if (found)
    {
        real_path = location.substr(best_length);
        real_path = server.root +"/"+ real_path;
        if (req.getMethod() == "GET"){
            size_t fileFd = open(real_path.c_str(), 0);
            if (fileFd == -1)
                found = false;
            else
            {
                close(fileFd);
            }
    }
    std::cout << "location root " << _location.root << std::endl;
    std::cout << "full path is " << real_path << " url is " << location << std::endl;
    }
    return found;
}

bool HttpResponce::validateMethod(){
    std::vector<std::string>::iterator it = _location.methods.begin();
    while (it != _location.methods.end()){
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
        real_path = server.error_pages[status_code];
    if (req.getMethod() == "GET")
    {
        size_t fileFd = open(real_path.c_str(), 0);
        size_t fileSize = read(fileFd, buff, 10000);
        fileSize = abs(fileSize);
        lenght = intToString(fileSize);
        }
    responce = "HTTP/1.1 " + status_message + "\r\nContent-Type: text/html\r\nContent-Length: "+ lenght + "\r\nConnection: keep-alive\r\n\r\n";
    responce += buff;
}