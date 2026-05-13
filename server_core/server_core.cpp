#include "server_core.hpp"
#include "../request-responce/HttpRequest.hpp"
#include "../request-responce/HttpResponce.hpp"

Server_core::Server_core(std::vector<Server>& servers) : serv(servers) {};

bool Server_core::is_server(int fd)
{
    for (int i = 0; i < sock_vector.size(); i++)
    {
        if (sock_vector[i] == fd)
            return (1);
    }
    return (0);
}

bool Server_core::server_init()
{
    for (size_t i = 0; i < serv.size(); i++)
    {
        server_id = socket(PF_INET, SOCK_STREAM, 0);
        client_len = sizeof(client_addr);
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(serv[i].port);
        server_addr.sin_addr.s_addr = INADDR_ANY;
        bind(server_id, (struct sockaddr*)&server_addr, sizeof(server_addr));
        listen(server_id, 10);
        sock_vector.push_back(server_id);
        tmp_pol.fd = server_id;
        tmp_pol.events = POLLIN;
        tmp_pol.revents = 0;
        vec_poll.push_back(tmp_pol);
    }
    return (1);
}

bool Server_core::client_connection(int server_fd)
{
    client_id = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_id > 0)
    {
        std::cout << "the client connected" << std::endl;
        tmp_pol.fd = client_id;
        tmp_pol.events = POLLIN;
        tmp_pol.revents = 0;
        vec_poll.push_back(tmp_pol);

        for (int i = 0; i < sock_vector.size(); i++)
        {
            if (sock_vector[i] == server_fd)
            {
                server_client[client_id] = &serv[i];
                break;
            }
        }
    }
    return (true);
}

bool Server_core::client_request(int id_client_req)
{
    char buffer[1024];
    int bytes;
    std::string tmp;
    std::string tmp2;
    size_t pos;
    bytes = recv(id_client_req, buffer, 1000, 0);
    if (bytes <= 0)
        return (0);
    buffer[bytes] = 0;
    tmp = buffer;
    req_map[id_client_req] += buffer;
    pos = req_map[id_client_req].find("\r\n\r\n");
    while (pos != std::string::npos)
    {
        tmp = req_map[id_client_req].substr(0, pos + 4);
        std::cout << tmp << std::endl;
        //jbelkerf part 
        HttpRequest req;
        tmp2 = buffer;
        req.feed(tmp, tmp2.substr(pos + 4));
        HttpResponce resp(req, *server_client[id_client_req]);
        std::string raw = resp.getResponce();
        send(id_client_req, raw.c_str(), raw.length(), 0);
        req_map[id_client_req].erase(0, pos + 4 + req.getContentLength());
        pos = req_map[id_client_req].find("\r\n\r\n");
    }
    return (1);
}


bool Server_core::server_starting()
{
    while (true)
    {
        poll(&vec_poll[0], vec_poll.size(), -1);

        for (int i = 0; i < vec_poll.size(); i++)
        {
            if (is_server(vec_poll[i].fd) && (vec_poll[i].revents & POLLIN))
                client_connection(vec_poll[i].fd);
            else if (vec_poll[i].revents & POLLIN)
            {
                if (!client_request(vec_poll[i].fd))
                    vec_poll.erase(vec_poll.begin() + i);
            }
        }
    }
    return (1);
}
