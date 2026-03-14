#include "server_core.hpp"

bool Server_core::server_init()
{
    server_id = socket(PF_INET, SOCK_STREAM, 0);
    client_len = sizeof(client_addr);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    bind(server_id, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_id, 10);
    return (1);
}

bool Server_core::client_connection()
{
    client_id = accept(server_id, (struct sockaddr*)&client_addr, &client_len);
    if (client_id > 0)
    {
        std::cout << "the client connected" << std::endl;
        tmp_pol.fd = client_id;
        tmp_pol.events = POLL_IN;
        tmp_pol.revents = 0;
        vec_poll.push_back(tmp_pol);
    }
    return (true);
}

bool Server_core::server_starting()
{
    tmp_pol.fd = server_id;
    tmp_pol.events = POLL_IN;
    tmp_pol.revents = 0;
    vec_poll.push_back(tmp_pol);

    while (true)
    {
        poll(&vec_poll[0], vec_poll.size(), -1);
        for (int i = 0; i < vec_poll.size(); i++)
        {
            if (vec_poll[i].fd == server_id && (vec_poll[i].events & POLL_IN))
                client_connection();
        }
    }
    return (1);
}