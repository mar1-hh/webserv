#include "HttpRequest.hpp"

void display_request(const HttpRequest &r)
{
    std::cout << "\n";
    std::cout << "┌─────────────────────────────────────────┐" << std::endl;
    std::cout << "│           HTTP REQUEST PARSED            │" << std::endl;
    std::cout << "└─────────────────────────────────────────┘" << std::endl;

    std::cout << "\n[ REQUEST LINE ]" << std::endl;
    std::cout << "  Method      : " << r.getMethod() << std::endl;
    std::cout << "  URI         : " << r.getUri() << std::endl;
    std::cout << "  Path        : " << r.getPath() << std::endl;
    std::cout << "  Query       : " << r.getQuery() << std::endl;
    std::cout << "  HTTP Ver    : " << r.getHttpVersion() << std::endl;

    std::cout << "\n[ HEADERS ]" << std::endl;
    std::map<std::string, std::string> headers = r.getHeaders();
    std::map<std::string, std::string>::iterator it;
    for (it = headers.begin(); it != headers.end(); it++)
        std::cout << "  '" << it->first << "' : " << it->second << std::endl;

    std::cout << "  Body    : " << r.getBody() << std::endl;
    std::cout << std::endl << std::endl;
}
/*
POST /submit HTTP/1.1\r\n                        ← request line
Host: localhost:8080\r\n                         ← header 1
Content-Type: application/x-www-form-urlencoded\r\n  ← header 2
Content-Length: 27\r\n                           ← header 3 (body is 27 bytes)
Connection: keep-alive\r\n                       ← header 4
\r\n                                             ← end of headers
username=jawad&password=1337                     ← body (exactly 27 bytes)
*/
// Ortodocs

HttpRequest::HttpRequest(){
    body = "";
    method = "";
    query = "";
    http_ver = "";
    is_chuncked = false;
    content_length = 0;

}

//exceptions 

const char * HttpRequest::MethodNotAllowed::what() const throw(){
    return "this method is now allowed";
}
const char *HttpRequest::VersionNotSupported::what() const throw()
{
    return "this version of http not supported";
}

void HttpRequest::parseRequestLine(){
    this->_state._state = REQUEST_LINE;
    size_t pos = _raw.find("\r\n");
    std::string line = _raw.substr(0, pos);

    pos = line.find(" ");
    std::string method = line.substr(0, pos);

    if (method == "GET" || method == "POST" || method == "DELETE" || method == "PUT" || method == "PATCH" ||\
            method == "HEAD" || method == "OPTIONS")
        this->method = method;
    else
    {
        this->_state._state = ERROR;
        this->_state._error = ERR_METHOD_NOT_ALLOWED;
        std::cout << "method " << method << "path " << path;
        return;
    }
    std::string newline = line.substr(pos + 1, line.length());
    pos = newline.find(" ");
    line = newline.substr(0, pos);
    pos = line.find("?");
    if (pos != std::string::npos)
    {
        this->uri = line.substr(0, pos);
        this->query = line.substr(pos + 1, line.length());
    }
    else
    {
        this->uri = line;
        this->query = "";
    }
    pos = newline.find(" ");
    line = newline.substr(pos + 1, newline.length());
    if (line == "HTTP/1.1" || line == "HTTP/1.0")
        this->http_ver = line;
    else
    {

        this->_state._state = ERROR;
        this->_state._error = ERR_VERSION_NOT_SUPPORTED;
        return;
    }
}
std::string getnextline(std::string &lines){
    std::string newline;
    size_t pos = lines.find("\r\n");
    newline = lines.substr(0, pos );
    lines = lines.substr(pos + 2, lines.length());
    return newline;
}

void HttpRequest::parseHeaders(){
    this->_state._state = HEADERS;
    std::string lines = _raw;
    std::string line;
    std::string key;
    std::string value;
    size_t pos = 1;
    // skipp first line
    getnextline(lines);

    while (pos != std::string::npos)
    {
        line = getnextline(lines);
        pos = line.find(":");
        if (pos != std::string::npos)
        {
            key = line.substr(0, pos);
            value = line.substr(pos + 1, line.length());
            if (value[0] == ' ')
                value = value.substr(1,value.length());
            this->headers[key] = value;
        }
    }
}
void HttpRequest::feed(std::string raw, std::string buffer)
{
    _raw = raw;
    parseRequestLine();
    parseHeaders();
    
    if (headers.find("Content-Length") != headers.end())
    {
        this->content_length = atoi(headers["Content-Length"].c_str());
        this->_state._state = BODY;
        std::string TransferEncoding = headers["Transfer-Encoding"];
        if (TransferEncoding == "chunked")
        {
            is_chuncked = true;
            return;
        }

        body = buffer.substr(0, content_length);
    }
    this->_state._state = COMPLETE;
    display_request(*this);
}

//getters
std::string HttpRequest::getMethod() const{
    return this->method;
}

std::string HttpRequest::getUri() const{
    return this->uri;
}
std::string HttpRequest::getPath() const
{
    return this->uri;
}

std::string HttpRequest::getHttpVersion() const{
    return this->http_ver;
}

std::string HttpRequest::getQuery() const{
    return this->query;
}
std::map<std::string, std::string> HttpRequest::getHeaders() const{
    return this->headers;
}
std::string HttpRequest::getBody() const{
    return this->body;
}

bool HttpRequest::isComplete() const{
    return (this->_state._state == COMPLETE);
}

bool HttpRequest::hasError() const{
    return this->_state._state == ERROR;
}

t_parseError HttpRequest::getError() const{
    return this->_state._error;
}

size_t HttpRequest::getContentLength() const{
    return this->content_length;
}

std::string HttpRequest::getHeader(const std::string &key) const{
    std::map<std::string, std::string>::const_iterator value_it = headers.find(key);
    if (value_it == headers.end())
        return "";
    return value_it->second;
}
