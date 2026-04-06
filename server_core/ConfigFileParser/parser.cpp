#include "parser.hpp"
#include "fstream"
#include <vector>

std::vector<std::string>split(const std::string line, char del)
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
bool is_nb(const std::string& nb)
{
    if (nb.empty()) {
        return false;
    }
    for (int i = 0; i < nb.size(); i++) {
        if (!isdigit(nb[i])) {
            return false;
        }
    }
    return true;
}
void parser(std::vector<Server>& servers)
{
    Location lc_data;
    Server sv_data;
    Server *currentServer = NULL;
    std::string line;
    std::ifstream file("config.conf");
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
        for (i = 0; i < (int)tokens.size(); i++)
        {
            if(tokens.empty())
            {
                std::cerr << "Error, Failed to get tokens" << std::endl;
                return;
            }
            if (tokens[i] == "server") {
                if (tokens.size() < i+2 || tokens[i+1] != "{") {
                    std::cerr << "Error, Bad token after server!" << std::endl;
                    return;
                }
                servers.push_back(Server());
                currentServer = &servers.back();
                state = IN_SERVER;
                i++;
            }
            else if (tokens[i] == "location") {
                if (servers.empty()) {
                    std::cerr << "Error, No Server yet!" << std::endl;
                    return;
                }
                if (tokens.size() < i+3 || tokens[i+2] != "{") {
                    std::cerr << "Error, Bad token after location!" << std::endl;
                    return;
                }
                lc_data = Location();
                lc_data.path = tokens[i + 1];
                currentServer->locations.push_back(lc_data);
                state = IN_LOCATION;
                i += 2;
            }
            else if (state == IN_SERVER && tokens[i] == "listen") {
                if (tokens.size() < i+3 || !is_nb(tokens[i+1]) || tokens[i+2] != ";") {
                    std::cerr << "Error, Bad listen directive!" << std::endl;
                    return;
                }
                currentServer->port = atoi(tokens[i+1].c_str());
                i += 2;
            }
            else if (state == IN_SERVER && tokens[i] == "interface") {
                if (tokens.size() < i+3 || tokens[i+2] != ";") {
                    std::cerr << "Error, Bad interface directive!" << std::endl;
                    return;
                }
                currentServer->interface_ip = tokens[i+1];
                i += 2;
            }
            else if (state == IN_SERVER && tokens[i] == "error_page") {
                if (tokens.size() < i+4 || !is_nb(tokens[i+1]) || tokens[i+3] != ";") {
                    std::cerr << "Error, Bad error_page directive!" << std::endl;
                    return;
                }
                int code = atoi(tokens[i+1].c_str());
                currentServer->error_pages[code] = tokens[i+2];
                i += 3;
            }
            else if (state == IN_SERVER && tokens[i] == "max_body_size") {
                if (tokens.size() < i+3 || !is_nb(tokens[i+1]) || tokens[i+2] != ";") {
                    std::cerr << "Error, Bad max_body_size directive!" << std::endl;
                    return;
                }
                currentServer->max_body_size = atoi(tokens[i+1].c_str());
                i += 2;
            }
            else if ((state == IN_SERVER || state == IN_LOCATION) && tokens[i] == "root") {
                if (tokens.size() < i+3 || tokens[i+2] != ";") {
                    std::cerr << "Error, Bad root directive!" << std::endl;
                    return;
                }
                if (state == IN_SERVER)
                    currentServer->root = tokens[i+1];
                else if (state == IN_LOCATION)
                    currentServer->locations.back().root = tokens[i+1];
                i += 2;
            }
            else if (state == IN_LOCATION && tokens[i] == "methods") {
                int j = i+1;
                std::vector<std::string> methods;
                while (j < (int)tokens.size() && tokens[j] != ";") {
                    methods.push_back(tokens[j]);
                    j++;
                }
                if (j == (int)tokens.size() || tokens[j] != ";") {
                    std::cerr << "Error, Bad methods directive!" << std::endl;
                    return;
                }
                currentServer->locations.back().methods = methods;
                i = j;
            }
            else if (state == IN_LOCATION && tokens[i] == "redirection") {
                if (tokens.size() < i+3 || tokens[i+2] != ";") {
                    std::cerr << "Error, Bad redirection directive!" << std::endl;
                    return;
                }
                currentServer->locations.back().redirection = tokens[i+1];
                i += 2;
            }
            else if (state == IN_LOCATION && tokens[i] == "directory_listing") {
                if (tokens.size() < i+3 || tokens[i+2] != ";") {
                    std::cerr << "Error, Bad directory_listing directive!" << std::endl;
                    return;
                }
                currentServer->locations.back().directory_listing = (tokens[i+1] == "on");
                i += 2;
            }
            else if (state == IN_LOCATION && tokens[i] == "default_file") {
                if (tokens.size() < i+3 || tokens[i+2] != ";") {
                    std::cerr << "Error, Bad default_file directive!" << std::endl;
                    return;
                }
                currentServer->locations.back().default_file = tokens[i+1];
                i += 2;
            }
            else if (state == IN_LOCATION && tokens[i] == "upload") {
                if (tokens.size() < i+4 || tokens[i+3] != ";") {
                    std::cerr << "Error, Bad upload directive!" << std::endl;
                    return;
                }
                currentServer->locations.back().upload_enabled = (tokens[i+1] == "on");
                currentServer->locations.back().upload_path = tokens[i+2];
                i += 3;
            }
            else if (state == IN_LOCATION && tokens[i] == "cgi") {
                if (tokens.size() < i+4 || tokens[i+3] != ";") {
                    std::cerr << "Error, Bad cgi directive!" << std::endl;
                    return;
                }
                currentServer->locations.back().cgi_extension = tokens[i+1];
                currentServer->locations.back().cgi_path = tokens[i+2];
                i += 3;
            }
            else if (tokens[i] == "}") {
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