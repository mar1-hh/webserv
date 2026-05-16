#include "HttpResponce.hpp"

//Helpers
std::string intToString(int n)
{
    std::ostringstream oss;
    oss << n;
    return oss.str();
}

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


// OCF 

HttpResponce::HttpResponce(HttpRequest &request, Server &serv): req(request), server(serv){
    status_code = 200;
    status_message = "HTTP/1.1 200 OK";
    fileContent = "";
    fileSize = 0;
    proccess();
    craftResponce();
    display_response(*this);
}




//validation
bool HttpResponce::validateLocation(){
    std::vector<Location> locations = server.locations;
    std::string location = req.getPath();
    size_t best_length =  0;
    bool found = false;
    
    std::vector<Location>::iterator it = locations.begin();
    while (it != locations.end()){
        
        if (location.find((*it).path) == 0)
        {
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
        std::string reqpath = req.getPath().substr(_location.path.length());
        real_path = server.root + _location.path + reqpath;
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




//GET 

void HttpResponce::handleListing(){
    struct dirent *entry;

    std::string directory = server.root+req.getPath();
    // if (server.root[server.root.length()-1] == '/' && req.getPath()[req.getPath().length() -1] == '/')
    //     di
    DIR *dir = opendir(directory.c_str());
    if (dir == NULL)
    {
        status_code = 404;
        status_message = "HTTP/1.1 404 Not Found";
        return;
    }
    fileContent = "<!DOCTYPE html><html><head><title> Files</title></head><body><h1> Directory Listing</h1><ul>";
    while ((entry = readdir(dir)) != NULL)
    {
        fileContent += "<li><a href=\"" + req.getPath() +"/"+entry->d_name+ "\">" + entry->d_name + "</a></li>";
    }
    fileContent += "</ul></body></html>";
    fileSize = fileContent.length();
}

void HttpResponce::handleindex(){
    if (real_path[real_path.length()-1] == '/')
        real_path +=  _location.default_file;
    else
    {
        real_path += "/" + _location.default_file;
    }
    
    readFile();
}

void HttpResponce::handleDir(){
    if (_location.default_file != "")
        handleindex();
    else if (_location.directory_listing)
        handleListing();
    else
    {
        status_code = 403;
        status_message = "HTTP/1.1 403 Forbidden";
    }
}

void HttpResponce::setFileType(){
    std::string file = real_path;
    contentType = "text/html";
    int pos2;

    size_t pos;
    pos = file.find(".");
    pos2 = pos;
    while (pos != std::string::npos)
    {
        pos2 = pos;
        file = file.substr(pos + 1);
        pos = file.find(".");
    }
    if(pos2 != std::string::npos)
    {
        std::string extention = req.getPath().substr(pos2);
        if (extention == ".html")
            contentType = "text/html";
        else if (extention == ".css")
            contentType = "text/css";
        else if (extention == ".js")
            contentType = "application/javascript";
        else if (extention == ".png")
            contentType = "image/png";
        else if (extention == ".jpeg")
            contentType = "image/jpeg";
        else if (extention == ".pdf")
            contentType = "application/pdf";
    }
}

void HttpResponce::readFile()
{
    struct stat sb;

    setFileType();
    int fileFd = open(real_path.c_str(), O_RDONLY);
    if (fileFd == -1)
    {
        status_code = 404;
        status_message = "HTTP/1.1 404 Not Found";
        return;
    }
    if (stat(real_path.c_str(), &sb) == -1)
    {
        close(fileFd);
        status_code = 500;
        status_message = "HTTP/1.1 500 Internal Server Error";
        return;
    }

    char chunk[8192];
    ssize_t readsize;

    while ((readsize = read(fileFd, chunk, sizeof(chunk))) > 0)
        fileContent.append(chunk, readsize);

    close(fileFd);

    if (readsize == -1)
    {
        fileContent.clear();
        fileSize = 0;
        status_code = 500;
        status_message = "HTTP/1.1 500 Internal Server Error";
        return;
    }
    fileSize = fileContent.length();
}

void HttpResponce::HandleGet(){
    struct stat sb;
    std::cout << "get trigried" << std::endl;

    if (stat(real_path.c_str(), &sb) == -1)
    {
        status_code = 404;
        status_message = "HTTP/1.1 404 Not Found";
        return;
    }
    if (S_ISDIR(sb.st_mode))
        handleDir();
    else if (S_ISREG(sb.st_mode))
        readFile();
    else
    {
        status_code = 404;
        status_message = "HTTP/1.1 404 Not Found";
    }
}

//POst
void HttpResponce::HandlePost(){
    std::cout << "post trigried" << std::endl;
    if (_location.upload_enabled == true)
    {
        real_path = server.root +"/"+ _location.upload_path + req.getPath().substr(_location.path.length() );
        std::cout << "the file to wriet is " << real_path << std::endl;
        int fileFd = open(real_path.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0777);
        if (fileFd == -1)
        {
            perror("open :");
            status_code = 500;
            status_message = "HTTP/1.1 500 Internal Server Error";
            return;
        }
        write(fileFd, req.getBody().c_str(), req.getBody().length());
        status_code = 201;
        status_message = "HTTP/1.1 201 Created";
        fileContent = status_message;
        fileSize = fileContent.length();
        close(fileFd);
        }
        else
        {
            status_code = 200;
            status_message = "HTTP/1.1 200 OK";
        }
}

//Delete
void HttpResponce::HandleDelete(){
    std::cout << "delete trigried" << std::endl;
    struct stat sb;

    if (stat(real_path.c_str(), &sb) == -1)
    {
        status_code = 404;
        status_message = "HTTP/1.1 404 Not Found";
    }
    else if (S_ISDIR(sb.st_mode))
    {
        status_code = 403;
        status_message = "HTTP/1.1 403 Forbidden";
    }
    else if (S_ISREG(sb.st_mode))
    {
        int status = unlink(real_path.c_str());
        if (status == -1)
        {
            status_code = 500;
            status_message = "HTTP/1.1 500 Internal Server Error";
        }
        else
        {
            status_code = 200;
            status_message = "HTTP/1.1 200 OK";
        }
    }
}

// The proccess
void HttpResponce::proccess(){
    status_message = "HTTP/1.1 200 OK";
    
    if (!validateLocation())
    {
        status_code = 404;
        status_message = "HTTP/1.1 404 Not Found";
            return;
    }
    if (_location.redirection != "")
    {
        status_code = 301;
        status_message = "HTTP/1.1 301 Moved Permanently";
        return;
    }
    if (!validateMethod())
    {
        status_code = 405;
        status_message = "HTTP/1.1 405 Method Not Allowed";
        return;
    }
    if (req.getMethod() == "POST" && atoi(req.getHeader("Content-Length").c_str()) >server.max_body_size)
    {
        status_code = 413;
        status_message = "HTTP/1.1 413 Content Too Large";
        return;
    }
    if (req.getMethod() == "GET")
        HandleGet();
    else if (req.getMethod() == "POST")
        HandlePost();
    else if (req.getMethod() == "DELETE")
        HandleDelete();
}


void HttpResponce::craftResponce(){
    if (status_code == 301)
    {
        status_message = "HTTP/1.1 301 Moved Permanently";
        responce = status_message + "\r\n" + "Location: " + _location.redirection + "\r\n"+"Content-Length: 0\r\n\r\n";
        return;
    }
    else if (status_code != 200 && status_code != 201)
    {
        std::map<int, std::string>::iterator it = server.error_pages.find(status_code);
        if (it != server.error_pages.end())
        {
            real_path = server.root +"/" +server.error_pages[status_code];
            int status = status_code;
            readFile();
            status_code = status;
        }
        else
        {
            fileContent = "<html><body><h1>" + status_message + "</h1></body></html>";
            fileSize = fileContent.length();
        }
        contentType = "text/html";
    }

    responce = status_message + "\r\n" + "Content-Type: "+ contentType + "\r\nContent-Length: " + intToString(fileSize) + "\r\n\r\n" + fileContent;

}

//getter 
std::string HttpResponce::getResponce() const{
    return responce;
}