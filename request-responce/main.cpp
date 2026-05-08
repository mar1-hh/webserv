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
}


int main() {
    std::string req = "POST /submit?a=b&c=d HTTP/1.1\r\nHost: localhost:8080\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: 27\r\nConnection: keep-alive\r\n\r\nusername=jawad&password=1337";

    HttpRequest r;
    try
    {
        r.feed(req);
        display_request(r);
    }
    catch( std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}
