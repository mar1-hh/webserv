#include "HttpResponce.hpp"
#include <sstream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

void debug(std::string a, std::string b){
    std::cout << a << "  " << b << std::endl;
}
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
    dircontent = "";
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
                found = true;
            }
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
                ;
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
        return true;
    while (it != _location.methods.end())
    {
        if (req.getMethod() == *it)
            return true;
        it++;
    }
    return false;
}

void HttpResponce::proccess(){
    status_message = "200 OK";
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
    if (req.getMethod() == "POST" && atoi(req.getHeader("Content-Length").c_str()) >server.max_body_size)
    {
        status_code = 413;
        status_message = "413 Content Too Large";
        return;
    }
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

void HttpResponce::handleListing(){
    struct dirent *entry;

    real_path += req.getPath();
    std::cout << "path is " << real_path << std::endl;
    DIR *dir = opendir(real_path.c_str());
    if (dir == NULL)
    {
        status_code = 404;
        return;
    }
    dircontent = "<!DOCTYPE html><html><head><title> Files</title></head><body><h1> Directory Listing</h1><ul>";
    while ((entry = readdir(dir)) != NULL)
    {
        dircontent += "<li><a href=\"" + _location.path +"/"+entry->d_name+ "\">" + entry->d_name + "</a></li>";
    }
    dircontent += "</ul></body></html>";
}

void HttpResponce::handleindex(){
    if (real_path[real_path.length() - 1] == '/')
        real_path[real_path.length() - 1] = '\0';
    real_path += req.getPath() + "/" + _location.default_file;
    std::cout << "path with index is " << real_path << std::endl;
}

void HttpResponce::handleDir(){
    debug("dir  endert", "  ");
    if (_location.directory_listing)
        return handleListing();
    handleindex();
    
}

void HttpResponce::craftResponce(){
    char buff[10001] = {0};
    std::string lenght;
    struct stat sb;

    stat(real_path.c_str(), &sb);
    if (S_ISDIR(sb.st_mode))
        handleDir();
    else if (S_ISREG(sb.st_mode))
        ;
    else
        status_code = 404;

    if (status_code != 200 && status_code != 301)
    {
        std::string errorpage = server.error_pages[status_code];
        if (errorpage == "")
            errorpage = server.error_pages[404];
        
        real_path = server.root + "/" + errorpage;
            
            
        std::cout << "error path is " << real_path << std::endl;
    }
    // handle redirection
    if (status_code == 301)
    {
        std::cout << "location " << _location.path << " redirection  " << _location.redirection << std::endl;
        status_message += "\r\nLocation: " + _location.redirection;
    }
    size_t fileSize;
    if (req.getMethod() == "GET")
    {
        size_t fileFd = open(real_path.c_str(), 0);
        fileSize = read(fileFd, buff, 10000);
        if (fileSize == -1)
            fileSize = 0;
        lenght = intToString(fileSize);
    }
    std::string buff2 = buff;
    if (dircontent != "")
    {
        buff2 = dircontent;
        fileSize = dircontent.length();
        lenght = intToString(fileSize);
    }
    responce = "HTTP/1.1 " + status_message + "\r\nContent-Type: text/html\r\nContent-Length: " + lenght + "\r\nConnection: keep-alive\r\n\r\n";
    responce += buff2;
}