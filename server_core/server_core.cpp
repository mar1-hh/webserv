#include "server_core.hpp"
#include "../request-responce/HttpRequest.hpp"
#include "../request-responce/HttpResponce.hpp"
#include "../cgi/cgi.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <signal.h>
#include <arpa/inet.h>

namespace
{
    const int REQ_INCOMPLETE = 0;
    const int REQ_READY = 1;
    const int REQ_BAD = 2;
    const int REQ_TOOLARGE = 3;

    const size_t MAX_HEADER = 16384;
    const time_t CGI_TIMEOUT = 10;
    const time_t CLIENT_TIMEOUT = 5;

    std::string toLower(const std::string &s)
    {
        std::string out = s;
        for (size_t i = 0; i < out.size(); i++)
            out[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(out[i])));
        return out;
    }

    std::string trim(const std::string &s)
    {
        size_t a = s.find_first_not_of(" \t");
        if (a == std::string::npos)
            return "";
        size_t b = s.find_last_not_of(" \t");
        return s.substr(a, b - a + 1);
    }

    void setNonBlocking(int fd)
    {
        fcntl(fd, F_SETFL, O_NONBLOCK);
    }

    in_addr_t parseHostAddr(const std::string &host)
    {
        if (host.empty() || host == "0.0.0.0")
            return INADDR_ANY;

        unsigned long parts[4];
        size_t idx = 0;
        std::string cur;
        for (size_t i = 0; i <= host.size(); i++)
        {
            if (i == host.size() || host[i] == '.')
            {
                if (cur.empty() || idx >= 4)
                    return INADDR_ANY;
                char *end;
                unsigned long v = std::strtoul(cur.c_str(), &end, 10);
                if (*end != '\0' || v > 255)
                    return INADDR_ANY;
                parts[idx++] = v;
                cur.clear();
            }
            else
                cur += host[i];
        }
        if (idx != 4)
            return INADDR_ANY;
        unsigned int addr = static_cast<unsigned int>(
            (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8) | parts[3]);
        return htonl(addr);
    }

    std::string ipToString(struct in_addr addr)
    {
        unsigned long n = ntohl(addr.s_addr);
        std::ostringstream oss;
        oss << ((n >> 24) & 0xff) << "." << ((n >> 16) & 0xff) << "."
            << ((n >> 8) & 0xff) << "." << (n & 0xff);
        return oss.str();
    }

    std::string reason(int code)
    {
        switch (code)
        {
        case 200: return "OK";
        case 500: return "Internal Server Error";
        case 502: return "Bad Gateway";
        case 504: return "Gateway Timeout";
        case 413: return "Payload Too Large";
        case 400: return "Bad Request";
        default: return "Error";
        }
    }

    std::string makeError(int code, const std::string &ver)
    {
        std::string r = reason(code);
        std::ostringstream body;
        body << "<!DOCTYPE html><html><head><title>" << code << "</title></head>"
             << "<body><h1>" << code << " " << r << "</h1></body></html>";
        std::string b = body.str();
        std::ostringstream out;
        out << ver << " " << code << " " << r << "\r\n"
            << "Content-Type: text/html\r\n"
            << "Content-Length: " << b.size() << "\r\n"
            << "Connection: close\r\n\r\n"
            << b;
        return out.str();
    }

    std::string buildCgiResponse(const std::string &output, const std::string &ver)
    {
        if (output.empty())
            return makeError(502, ver);

        size_t sep = output.find("\r\n\r\n");
        size_t seplen = 4;
        if (sep == std::string::npos)
        {
            sep = output.find("\n\n");
            seplen = 2;
        }

        std::string headerPart, body;
        if (sep != std::string::npos)
        {
            headerPart = output.substr(0, sep);
            body = output.substr(sep + seplen);
        }
        else
            body = output;

        int code = 200;
        std::string phrase = "OK";
        std::string passHeaders;
        bool hasType = false;

        std::istringstream hs(headerPart);
        std::string line;
        while (std::getline(hs, line))
        {
            if (!line.empty() && line[line.size() - 1] == '\r')
                line.erase(line.size() - 1);
            if (line.empty())
                continue;

            std::string lower = toLower(line);
            if (lower.compare(0, 7, "status:") == 0)
            {
                std::string val = trim(line.substr(7));
                code = std::atoi(val.c_str());
                size_t sp = val.find(' ');
                phrase = (sp == std::string::npos) ? "" : val.substr(sp + 1);
                if (code == 0)
                    code = 200;
                continue;
            }
            if (lower.compare(0, 15, "content-length:") == 0)
                continue;
            if (lower.compare(0, 13, "content-type:") == 0)
                hasType = true;
            passHeaders += line + "\r\n";
        }

        std::ostringstream out;
        out << ver << " " << code << " " << (phrase.empty() ? reason(code) : phrase) << "\r\n";
        out << passHeaders;
        if (!hasType)
            out << "Content-Type: text/html\r\n";
        out << "Content-Length: " << body.size() << "\r\n";
        out << "Connection: close\r\n\r\n";
        out << body;
        return out.str();
    }

    void splitPath(const std::string &full, std::string &dir, std::string &base)
    {
        size_t slash = full.rfind('/');
        if (slash == std::string::npos)
        {
            dir = "";
            base = full;
        }
        else
        {
            dir = full.substr(0, slash);
            base = full.substr(slash + 1);
        }
    }

    char **toCArray(const std::vector<std::string> &v)
    {
        char **arr = new char *[v.size() + 1];
        for (size_t i = 0; i < v.size(); i++)
        {
            arr[i] = new char[v[i].size() + 1];
            std::memcpy(arr[i], v[i].c_str(), v[i].size() + 1);
        }
        arr[v.size()] = NULL;
        return arr;
    }
}

Connection::Connection()
    : fd(-1), server(NULL), out_sent(0), state(C_READING), last_active(0), drain(false),
      cgi_pid(-1), cgi_in_fd(-1), cgi_out_fd(-1), cgi_in_off(0), cgi_start(0)
{
}

Server_core::Server_core(std::vector<Server> &servers) : serv(servers) {}

Server_core::~Server_core()
{
    std::map<int, Connection *>::iterator it;
    for (it = conns.begin(); it != conns.end(); ++it)
    {
        close(it->first);
        delete it->second;
    }
    for (std::map<int, Server *>::iterator l = listeners.begin(); l != listeners.end(); ++l)
        close(l->first);
}

void Server_core::addPoll(int fd, short events)
{
    struct pollfd p;
    p.fd = fd;
    p.events = events;
    p.revents = 0;
    pfds.push_back(p);
}

void Server_core::removePoll(int fd)
{
    for (size_t i = 0; i < pfds.size(); i++)
    {
        if (pfds[i].fd == fd)
        {
            pfds.erase(pfds.begin() + i);
            return;
        }
    }
}

void Server_core::setPollEvents(int fd, short events)
{
    for (size_t i = 0; i < pfds.size(); i++)
    {
        if (pfds[i].fd == fd)
        {
            pfds[i].events = events;
            return;
        }
    }
}

bool Server_core::init()
{
    for (size_t i = 0; i < serv.size(); i++)
    {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0)
        {
            std::cerr << "Error: socket() failed" << std::endl;
            continue;
        }
        int opt = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        setNonBlocking(fd);

        struct sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<unsigned short>(serv[i].port));
        addr.sin_addr.s_addr = parseHostAddr(serv[i].host);

        if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        {
            std::cerr << "Error: bind() failed on " << serv[i].host << ":" << serv[i].port << std::endl;
            close(fd);
            continue;
        }
        if (listen(fd, SOMAXCONN) < 0)
        {
            std::cerr << "Error: listen() failed on port " << serv[i].port << std::endl;
            close(fd);
            continue;
        }
        listeners[fd] = &serv[i];
        addPoll(fd, POLLIN);
        std::cout << "Listening on " << serv[i].host << ":" << serv[i].port << std::endl;
    }
    if (listeners.empty())
    {
        std::cerr << "Error: no listening socket could be created" << std::endl;
        return false;
    }
    return true;
}

void Server_core::run()
{
    while (true)
    {
        int ready = poll(&pfds[0], pfds.size(), 1000);
        if (ready < 0)
        {
            checkTimeouts();
            continue;
        }

        std::vector<struct pollfd> snap = pfds;
        for (size_t i = 0; i < snap.size(); i++)
        {
            short re = snap[i].revents;
            if (re == 0)
                continue;
            int fd = snap[i].fd;

            if (listeners.count(fd))
            {
                if (re & POLLIN)
                    acceptClient(fd);
            }
            else if (cgi_out_client.count(fd))
                onCgiRead(fd);
            else if (cgi_in_client.count(fd))
                onCgiWrite(fd);
            else if (conns.count(fd))
            {
                if (re & POLLOUT)
                    onClientWrite(fd);
                else if (re & POLLIN)
                    onClientRead(fd);
                else if (re & (POLLERR | POLLHUP | POLLNVAL))
                    closeClient(fd);
            }
        }
        checkTimeouts();
    }
}

void Server_core::acceptClient(int lfd)
{
    while (true)
    {
        struct sockaddr_in caddr;
        socklen_t clen = sizeof(caddr);
        int c = accept(lfd, (struct sockaddr *)&caddr, &clen);
        if (c < 0)
            break;
        setNonBlocking(c);
        Connection *conn = new Connection();
        conn->fd = c;
        conn->server = listeners[lfd];
        conn->client_ip = ipToString(caddr.sin_addr);
        conn->state = C_READING;
        conn->last_active = std::time(NULL);
        conns[c] = conn;
        addPoll(c, POLLIN);
    }
}

int Server_core::requestStatus(Connection *c, size_t &reqLen)
{
    const std::string &b = c->inbuf;
    size_t he = b.find("\r\n\r\n");
    if (he == std::string::npos)
    {
        if (b.size() > MAX_HEADER)
            return REQ_BAD;
        return REQ_INCOMPLETE;
    }

    size_t headerLen = he + 4;
    size_t maxBody = c->server->max_body_size;
    std::string head = b.substr(0, he);
    std::string lower = toLower(head);

    if (lower.find("transfer-encoding:") != std::string::npos &&
        lower.find("chunked") != std::string::npos)
    {
        size_t term = b.find("0\r\n\r\n", headerLen);
        if (term == std::string::npos)
        {
            if (maxBody && b.size() - headerLen > maxBody)
                return REQ_TOOLARGE;
            return REQ_INCOMPLETE;
        }
        reqLen = term + 5;
        return REQ_READY;
    }

    size_t clPos = lower.find("content-length:");
    if (clPos != std::string::npos)
    {
        size_t vstart = clPos + 15;
        size_t vend = head.find("\r\n", vstart);
        std::string val = head.substr(vstart, (vend == std::string::npos ? head.size() : vend) - vstart);
        long cl = std::atol(trim(val).c_str());
        if (cl < 0)
            cl = 0;
        if (maxBody && static_cast<size_t>(cl) > maxBody)
            return REQ_TOOLARGE;
        if (b.size() < headerLen + static_cast<size_t>(cl))
            return REQ_INCOMPLETE;
        reqLen = headerLen + static_cast<size_t>(cl);
        return REQ_READY;
    }

    reqLen = headerLen;
    return REQ_READY;
}

void Server_core::onClientRead(int fd)
{
    Connection *c = conns[fd];

    if (c->state == C_DRAIN)
    {
        char buf[8192];
        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        if (n <= 0)
            closeClient(fd);
        return;
    }

    char buf[8192];
    ssize_t n = recv(fd, buf, sizeof(buf), 0);
    if (n <= 0)
    {
        closeClient(fd);
        return;
    }
    c->inbuf.append(buf, n);
    c->last_active = std::time(NULL);

    size_t reqLen = 0;
    int st = requestStatus(c, reqLen);
    if (st == REQ_INCOMPLETE)
        return;
    if (st == REQ_TOOLARGE || st == REQ_BAD)
    {
        c->outbuf = makeError(st == REQ_TOOLARGE ? 413 : 400, "HTTP/1.1");
        c->out_sent = 0;
        c->state = C_WRITING;
        c->drain = true;
        setPollEvents(fd, POLLOUT);
        return;
    }
    processRequest(c, c->inbuf.substr(0, reqLen));
}

void Server_core::processRequest(Connection *c, const std::string &full)
{
    HttpRequest req;
    req.feed(full);

    HttpResponce resp(req, *c->server, sessions);
    resp.process();

    if (resp.isCgi())
    {
        std::ostringstream port;
        port << c->server->port;
        std::vector<std::string> env = build_cgi_env(req, resp.cgiScriptName(),
                                                      resp.cgiScript(), resp.cgiPathInfo(),
                                                      c->server->host, port.str(), c->client_ip);
        startCgi(c, resp.cgiInterpreter(), resp.cgiScript(), env, req.getBody(), req.getHttpVersion());
        return;
    }

    c->outbuf = resp.getResponce();
    c->out_sent = 0;
    c->state = C_WRITING;
    setPollEvents(c->fd, POLLOUT);
}

void Server_core::startCgi(Connection *c, const std::string &interp, const std::string &script,
                           const std::vector<std::string> &env, const std::string &body,
                           const std::string &httpVer)
{
    int in_pipe[2];
    int out_pipe[2];
    if (pipe(in_pipe) < 0)
    {
        c->outbuf = makeError(500, "HTTP/1.1");
        c->out_sent = 0;
        c->state = C_WRITING;
        setPollEvents(c->fd, POLLOUT);
        return;
    }
    if (pipe(out_pipe) < 0)
    {
        close(in_pipe[0]);
        close(in_pipe[1]);
        c->outbuf = makeError(500, "HTTP/1.1");
        c->out_sent = 0;
        c->state = C_WRITING;
        setPollEvents(c->fd, POLLOUT);
        return;
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        close(in_pipe[0]);
        close(in_pipe[1]);
        close(out_pipe[0]);
        close(out_pipe[1]);
        c->outbuf = makeError(500, "HTTP/1.1");
        c->out_sent = 0;
        c->state = C_WRITING;
        setPollEvents(c->fd, POLLOUT);
        return;
    }

    if (pid == 0)
    {
        dup2(in_pipe[0], STDIN_FILENO);
        dup2(out_pipe[1], STDOUT_FILENO);
        close(in_pipe[0]);
        close(in_pipe[1]);
        close(out_pipe[0]);
        close(out_pipe[1]);

        std::string dir, base;
        splitPath(script, dir, base);
        if (!dir.empty())
        {
            if (chdir(dir.c_str()) != 0)
                _exit(1);
        }

        std::vector<std::string> argvec;
        argvec.push_back(interp);
        argvec.push_back(base);
        char **argv = toCArray(argvec);
        char **envp = toCArray(env);
        execve(interp.c_str(), argv, envp);
        _exit(1);
    }

    close(in_pipe[0]);
    close(out_pipe[1]);
    setNonBlocking(in_pipe[1]);
    setNonBlocking(out_pipe[0]);

    c->cgi_pid = pid;
    c->cgi_out_fd = out_pipe[0];
    c->cgi_input = body;
    c->cgi_in_off = 0;
    c->cgi_output.clear();
    c->http_ver = httpVer.empty() ? "HTTP/1.1" : httpVer;
    c->cgi_start = std::time(NULL);
    c->state = C_CGI;
    setPollEvents(c->fd, 0);

    addPoll(out_pipe[0], POLLIN);
    cgi_out_client[out_pipe[0]] = c->fd;

    if (!body.empty())
    {
        c->cgi_in_fd = in_pipe[1];
        addPoll(in_pipe[1], POLLOUT);
        cgi_in_client[in_pipe[1]] = c->fd;
    }
    else
        close(in_pipe[1]);
}

void Server_core::onCgiWrite(int fd)
{
    int clientFd = cgi_in_client[fd];
    std::map<int, Connection *>::iterator cit = conns.find(clientFd);
    if (cit == conns.end())
    {
        close(fd);
        removePoll(fd);
        cgi_in_client.erase(fd);
        return;
    }
    Connection *c = cit->second;
    size_t remain = c->cgi_input.size() - c->cgi_in_off;
    ssize_t n = write(fd, c->cgi_input.data() + c->cgi_in_off, remain);
    if (n <= 0)
    {
        close(fd);
        removePoll(fd);
        cgi_in_client.erase(fd);
        c->cgi_in_fd = -1;
        return;
    }
    c->cgi_in_off += n;
    if (c->cgi_in_off >= c->cgi_input.size())
    {
        close(fd);
        removePoll(fd);
        cgi_in_client.erase(fd);
        c->cgi_in_fd = -1;
    }
}

void Server_core::onCgiRead(int fd)
{
    int clientFd = cgi_out_client[fd];
    std::map<int, Connection *>::iterator cit = conns.find(clientFd);
    if (cit == conns.end())
    {
        close(fd);
        removePoll(fd);
        cgi_out_client.erase(fd);
        return;
    }
    Connection *c = cit->second;

    char buf[8192];
    ssize_t n = read(fd, buf, sizeof(buf));
    if (n > 0)
    {
        c->cgi_output.append(buf, n);
        return;
    }

    close(fd);
    removePoll(fd);
    cgi_out_client.erase(fd);
    if (c->cgi_in_fd != -1)
    {
        removePoll(c->cgi_in_fd);
        cgi_in_client.erase(c->cgi_in_fd);
        close(c->cgi_in_fd);
        c->cgi_in_fd = -1;
    }
    if (c->cgi_pid > 0)
    {
        int status;
        waitpid(c->cgi_pid, &status, 0);
        c->cgi_pid = -1;
    }

    c->outbuf = buildCgiResponse(c->cgi_output, c->http_ver);
    c->out_sent = 0;
    c->state = C_WRITING;
    setPollEvents(c->fd, POLLOUT);
}

void Server_core::onClientWrite(int fd)
{
    Connection *c = conns[fd];
    size_t remain = c->outbuf.size() - c->out_sent;
    ssize_t n = send(fd, c->outbuf.data() + c->out_sent, remain, 0);
    if (n <= 0)
    {
        closeClient(fd);
        return;
    }
    c->out_sent += n;
    if (c->out_sent >= c->outbuf.size())
    {
        if (c->drain)
        {
            shutdown(fd, SHUT_WR);
            c->state = C_DRAIN;
            c->last_active = std::time(NULL);
            setPollEvents(fd, POLLIN);
        }
        else
            closeClient(fd);
    }
}

void Server_core::cleanupCgi(Connection *c)
{
    if (c->cgi_in_fd != -1)
    {
        removePoll(c->cgi_in_fd);
        cgi_in_client.erase(c->cgi_in_fd);
        close(c->cgi_in_fd);
        c->cgi_in_fd = -1;
    }
    if (c->cgi_out_fd != -1)
    {
        removePoll(c->cgi_out_fd);
        cgi_out_client.erase(c->cgi_out_fd);
        close(c->cgi_out_fd);
        c->cgi_out_fd = -1;
    }
    if (c->cgi_pid > 0)
    {
        kill(c->cgi_pid, SIGKILL);
        int status;
        waitpid(c->cgi_pid, &status, 0);
        c->cgi_pid = -1;
    }
}

void Server_core::closeClient(int fd)
{
    std::map<int, Connection *>::iterator it = conns.find(fd);
    if (it == conns.end())
        return;
    Connection *c = it->second;
    cleanupCgi(c);
    removePoll(fd);
    close(fd);
    conns.erase(it);
    delete c;
}

void Server_core::checkTimeouts()
{
    time_t now = std::time(NULL);
    std::vector<int> kill_cgi;
    std::vector<int> drop;

    std::map<int, Connection *>::iterator it;
    for (it = conns.begin(); it != conns.end(); ++it)
    {
        Connection *c = it->second;
        if (c->state == C_CGI && now - c->cgi_start > CGI_TIMEOUT)
            kill_cgi.push_back(it->first);
        else if ((c->state == C_READING || c->state == C_DRAIN) && now - c->last_active >= CLIENT_TIMEOUT)
            drop.push_back(it->first);
    }

    for (size_t i = 0; i < kill_cgi.size(); i++)
    {
        std::map<int, Connection *>::iterator c = conns.find(kill_cgi[i]);
        if (c == conns.end())
            continue;
        cleanupCgi(c->second);
        c->second->outbuf = makeError(504, c->second->http_ver);
        c->second->out_sent = 0;
        c->second->state = C_WRITING;
        setPollEvents(c->first, POLLOUT);
    }
    for (size_t i = 0; i < drop.size(); i++)
        closeClient(drop[i]);
}
