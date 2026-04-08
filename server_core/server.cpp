#include "server_core.hpp"

int main()
{
    std::vector<Server> serv;
    parser(serv);
    Server_core s(serv);
    s.server_init();
    s.server_starting();
    return (0);
}