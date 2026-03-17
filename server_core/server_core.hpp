#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <vector>
#include <map>


class Server_core
{
    private:
    int server_id;
    int client_id;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t   client_len;
    struct pollfd   tmp_pol;
    std::vector<struct pollfd>  vec_poll;
    std::map<int, std::string> req_map;
    bool client_connection();
    bool client_request(int id_client_req);


    public:
    bool server_init();
    bool server_starting();
    // ~Server_core();
    

};