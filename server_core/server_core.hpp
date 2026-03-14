#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <vector>

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
    bool client_connection();

    public:
    bool server_init();
    bool server_starting();
    // ~Server_core();

};