#include "HttpRequest.hpp"




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
