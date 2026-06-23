#ifndef SERVER_CORE_HPP
#define SERVER_CORE_HPP

#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <poll.h>
#include <sys/types.h>
#include <netinet/in.h>
#include "ConfigFileParser/parser.hpp"
#include "../request-responce/Session.hpp"

enum ConnState
{
    C_READING,
    C_CGI,
    C_WRITING,
    C_DRAIN
};

struct Connection
{
    int fd;
    Server *server;
    std::string client_ip;
    std::string inbuf;
    std::string outbuf;
    size_t out_sent;
    ConnState state;
    time_t last_active;
    bool drain;

    pid_t cgi_pid;
    int cgi_in_fd;
    int cgi_out_fd;
    std::string cgi_input;
    size_t cgi_in_off;
    std::string cgi_output;
    std::string http_ver;
    time_t cgi_start;

    Connection();
};

class Server_core
{
private:
    std::vector<Server> &serv;
    std::vector<struct pollfd> pfds;
    std::map<int, Server *> listeners;
    std::map<int, Connection *> conns;
    std::map<int, int> cgi_out_client;
    std::map<int, int> cgi_in_client;
    SessionManager sessions;

    void addPoll(int fd, short events);
    void removePoll(int fd);
    void setPollEvents(int fd, short events);

    void acceptClient(int lfd);
    void onClientRead(int fd);
    void onClientWrite(int fd);
    void onCgiRead(int fd);
    void onCgiWrite(int fd);

    int requestStatus(Connection *c, size_t &reqLen);
    void processRequest(Connection *c, const std::string &full);
    void startCgi(Connection *c, const std::string &interp, const std::string &script,
                  const std::vector<std::string> &env, const std::string &body,
                  const std::string &httpVer);
    void cleanupCgi(Connection *c);
    void closeClient(int fd);
    void checkTimeouts();

public:
    Server_core(std::vector<Server> &servers);
    ~Server_core();

    bool init();
    void run();
};

#endif
