#include "parser.hpp"
#include "fstream"
#include <vector>

std::vector<std::string>split(std::string line, char del)
{
    char now;
    int i = 0;

    std::string token;
    std::vector<std::string> tokeens;
    while(i < line.size())
    {
        token += line[i];
        if (line[i] == del) {
            tokeens.push_back(token);
            token.clear();
        }
        else if (line[i] == ';' || line[i] == '{' || line[i] == '}')
        {
            if (!token.empty()) {
                tokeens.push_back(token);
                token.clear();
            }
            tokeens.push_back(std::string(1, line[i]));
        }
        else
            token += line[i];
        i++;
    }
    if (!token.empty())
        tokeens.push_back(token);
    return tokeens;
}
enum states{
    GLOBAL,
    IN_SERVER,
    IN_LOCATION
};
bool is_nb(std::string& nb)
{
    if (nb.empty()) {
        return false;
    }
    for (int i = 0; i < nb.size(); i++) {
        if (!isdigit(nb[i])) {
            return false;
        }
    }
    return false;
}
void parser(std::vector<Server>& servers)
{
    Location lc_data;
    Server sv_data;
    Server *currentServer;
    std::string line;
    std::ifstream file("config.conf");
    bool inside_ser = 0;
    states state = GLOBAL;
    if(!file.is_open())
    {
        std::cerr << "Error, Cant open the file" << std::endl;
        return;
    }
    int i;
    while (getline(file, line)) {
        i = 0;
        std::vector<std::string> tokens = split(line, ' ');
        for (int i = 0; i < tokens.size(); i++)
        {
            if(tokens.empty())
            {
                std::cerr << "Error, Failed to get tokens" << std::endl;
                return;
            }
            if (tokens[i] == "server") {
                if (tokens.size() != i+2 || tokens[i+1] != "{") {
                    std::cerr << "Error, Bad token!" << std::endl;
                    return;
                }
                servers.push_back(sv_data);
                currentServer = &servers.back();
                state = IN_SERVER;
                i++;
            }
            if (tokens[i] == "location") {
                if (servers.empty()) {
                    std::cerr << "Error, No Server yet!" << std::endl;
                    return;
                }
                if (tokens.size() < 3 || tokens[i+2] != "{") {
                    std::cerr << "Error, Bad token!" << std::endl;
                    return;
                }
                currentServer = &servers.back();
                lc_data.path = tokens[i + 1];
                currentServer->locations.push_back(lc_data);
                state = IN_LOCATION;
                i++;
            }
            if (state == IN_SERVER) {
                if (tokens[i] == "listen") {
                    if (!is_nb(tokens[i+1]) || tokens[i+2] != ";") {
                        std::cerr << "Error, Bad token!" << std::endl;
                        return;
                    }
                    currentServer = &servers.back();
                    currentServer->port = atoi(tokens[i+1].c_str());
                }
                if (tokens[i] == "root") {
                    if (tokens[i+2] != ";") {
                        std::cerr << "Error, Bad token!" << std::endl;
                        return;
                    }
                    currentServer = &servers.back();
                    currentServer->root = tokens[i+1];
                }
            }
            if (tokens[i] == "}") {
                i++;
                if (state == IN_LOCATION)
                    state = IN_SERVER;
                else if (state == IN_SERVER)
                    state = GLOBAL;
                else
                {
                    std::cerr << "Error, Bad token!" << std::endl;
                    return;
                }
            }
        
        }

    }


    file.close();
}