#include "server_core.hpp"
#include <csignal>
#include <string>
#include <vector>

int main(int ac, char **av)
{
    if (ac > 2)
    {
        std::cerr << "Usage: ./webserv [configuration file]" << std::endl;
        return 1;
    }

    std::signal(SIGPIPE, SIG_IGN);

    std::string path = (ac == 2) ? av[1] : "config.conf";
    std::vector<Server> servers;
    if (!parser(servers, path))
        return 1;

    Server_core core(servers);
    if (!core.init())
        return 1;
    core.run();
    return 0;
}
