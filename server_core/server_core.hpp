#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <vector>
#include <map>
#include "ConfigFileParser/parser.hpp"


class Server_core
{
    private:
    std::vector<Server>& serv;
    std::vector<int> sock_vector;
    int server_id;
    int client_id;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t   client_len;
    struct pollfd   tmp_pol;
    std::vector<struct pollfd>  vec_poll;
    std::map<int, std::string> req_map;
    std::map<int, Server*> server_client;
    bool client_connection(int server_fd);
    bool client_request(int id_client_req);


    public:
    Server_core(std::vector<Server>& servers);
    bool server_init();
    bool server_starting();
    bool is_server(int fd);
    // ~Server_core();
    

};