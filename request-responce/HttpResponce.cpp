#include "HttpResponce.hpp"

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

    std::vector<Location>::iterator it = locations.begin();
    while (it != locations.end()){
        if (location == (*it).path)
        {
            _location = *it;
            return true;
        }
        it++;
    }
    return false;
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

void HttpResponce::craftResponce(){
    responce = "HTTP/1.1 " + status_message + "\r\nContent-Type: text/html\r\nContent-Length: 13\r\nConnection: keep-alive\r\n\r\n";
    responce += "file content\n";
}