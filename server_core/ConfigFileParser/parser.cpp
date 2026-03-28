#include "parser.hpp"

#include "fstream"

#include <vector>

std::vector<std::string>split(std::string line, char del)
{
    char now;
    int i = 0;

    std::string token;
    std::vector<std::string> tokeens;
    while(line[i] != '\0')
    {
        token += line[i];
        if (line[i] == del) {
            tokeens.push_back(token);
            token.clear();
        }
    }
    if (line[i] != '\0' && line[i - 1] != del) {
        tokeens.push_back(token);
        token.clear();
    }
    return tokeens;
}
enum states{
    GLOBAL,
    INSERVER,

}
void parser(std::vector<Server> servers)
{
    Location lc_data;
    Server sv_data;
    std::string line;
    std::ifstream file("config.conf");
    bool inside_ser = 0;
    if(!file.is_open())
    {
        std::cerr << "Error, Cant open the file" << std::endl;
        return;
    }
    while (!line.empty()) {
        getline(file, line);
        std::vector<std::string> tokens = split(line, '\n');
        if(tokens.empty())
        {
            std::cerr << "Error, Failed to get tokens" << std::endl;
            return;
        }
        if (tokens[0] == "server") {
            if (tokens.size() != 2 || tokens[1] != "{") {
                throw std::runtime_error("Invalid server block");
            }

            servers.push_back(sv_data);
            currentServer = &servers.back();
            state = IN_SERVER;
        }
    }






    file.close();
}