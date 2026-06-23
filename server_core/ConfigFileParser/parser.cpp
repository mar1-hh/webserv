#include "parser.hpp"
#include <fstream>
#include <cstdlib>
#include <cctype>

Location::Location()
    : directory_listing(false), upload_enabled(false)
{
}

Server::Server()
    : host("0.0.0.0"), port(80), max_body_size(1048576)
{
}

static std::vector<std::string> split(const std::string &line)
{
    std::string token;
    std::vector<std::string> tokens;

    for (size_t i = 0; i < line.size(); i++)
    {
        char c = line[i];
        if (c == ' ' || c == '\t')
        {
            if (!token.empty())
            {
                tokens.push_back(token);
                token.clear();
            }
        }
        else if (c == ';' || c == '{' || c == '}')
        {
            if (!token.empty())
            {
                tokens.push_back(token);
                token.clear();
            }
            tokens.push_back(std::string(1, c));
        }
        else
            token += c;
    }
    if (!token.empty())
        tokens.push_back(token);
    return tokens;
}

enum States
{
    GLOBAL,
    IN_SERVER,
    IN_LOCATION
};

static bool is_number(const std::string &nb)
{
    if (nb.empty())
        return false;
    for (size_t i = 0; i < nb.size(); i++)
    {
        if (!std::isdigit(static_cast<unsigned char>(nb[i])))
            return false;
    }
    return true;
}

bool parser(std::vector<Server> &servers, const std::string &path)
{
    Server *current = NULL;
    std::string line;
    std::ifstream file(path.c_str());
    States state = GLOBAL;

    if (!file.is_open())
    {
        std::cerr << "Error: cannot open config file: " << path << std::endl;
        return false;
    }

    while (std::getline(file, line))
    {
        std::vector<std::string> tokens = split(line);
        for (size_t i = 0; i < tokens.size(); i++)
        {
            const std::string &tok = tokens[i];

            if (tok == "server")
            {
                if (state != GLOBAL || i + 1 >= tokens.size() || tokens[i + 1] != "{")
                {
                    std::cerr << "Error: bad token after 'server'" << std::endl;
                    return false;
                }
                servers.push_back(Server());
                current = &servers.back();
                state = IN_SERVER;
                i++;
            }
            else if (tok == "location")
            {
                if (state != IN_SERVER || i + 2 >= tokens.size() || tokens[i + 2] != "{")
                {
                    std::cerr << "Error: bad token after 'location'" << std::endl;
                    return false;
                }
                Location loc;
                loc.path = tokens[i + 1];
                current->locations.push_back(loc);
                state = IN_LOCATION;
                i += 2;
            }
            else if (state == IN_SERVER && (tok == "host" || tok == "interface"))
            {
                if (i + 2 >= tokens.size() || tokens[i + 2] != ";")
                {
                    std::cerr << "Error: bad host directive" << std::endl;
                    return false;
                }
                current->host = tokens[i + 1];
                i += 2;
            }
            else if (state == IN_SERVER && tok == "listen")
            {
                if (i + 2 >= tokens.size() || !is_number(tokens[i + 1]) || tokens[i + 2] != ";")
                {
                    std::cerr << "Error: bad listen directive" << std::endl;
                    return false;
                }
                current->port = std::atoi(tokens[i + 1].c_str());
                i += 2;
            }
            else if (state == IN_SERVER && tok == "error_page")
            {
                if (i + 3 >= tokens.size() || !is_number(tokens[i + 1]) || tokens[i + 3] != ";")
                {
                    std::cerr << "Error: bad error_page directive" << std::endl;
                    return false;
                }
                current->error_pages[std::atoi(tokens[i + 1].c_str())] = tokens[i + 2];
                i += 3;
            }
            else if (state == IN_SERVER && tok == "max_body_size")
            {
                if (i + 2 >= tokens.size() || !is_number(tokens[i + 1]) || tokens[i + 2] != ";")
                {
                    std::cerr << "Error: bad max_body_size directive" << std::endl;
                    return false;
                }
                current->max_body_size = static_cast<size_t>(std::atol(tokens[i + 1].c_str()));
                i += 2;
            }
            else if ((state == IN_SERVER || state == IN_LOCATION) && tok == "root")
            {
                if (i + 2 >= tokens.size() || tokens[i + 2] != ";")
                {
                    std::cerr << "Error: bad root directive" << std::endl;
                    return false;
                }
                if (state == IN_SERVER)
                    current->root = tokens[i + 1];
                else
                    current->locations.back().root = tokens[i + 1];
                i += 2;
            }
            else if (state == IN_LOCATION && tok == "methods")
            {
                size_t j = i + 1;
                std::vector<std::string> methods;
                while (j < tokens.size() && tokens[j] != ";")
                    methods.push_back(tokens[j++]);
                if (j >= tokens.size())
                {
                    std::cerr << "Error: bad methods directive" << std::endl;
                    return false;
                }
                current->locations.back().methods = methods;
                i = j;
            }
            else if (state == IN_LOCATION && tok == "redirection")
            {
                if (i + 2 >= tokens.size() || tokens[i + 2] != ";")
                {
                    std::cerr << "Error: bad redirection directive" << std::endl;
                    return false;
                }
                current->locations.back().redirection = tokens[i + 1];
                i += 2;
            }
            else if (state == IN_LOCATION && (tok == "directory_listing" || tok == "autoindex"))
            {
                if (i + 2 >= tokens.size() || tokens[i + 2] != ";")
                {
                    std::cerr << "Error: bad directory listing directive" << std::endl;
                    return false;
                }
                current->locations.back().directory_listing = (tokens[i + 1] == "on");
                i += 2;
            }
            else if (state == IN_LOCATION && tok == "index")
            {
                if (i + 2 >= tokens.size() || tokens[i + 2] != ";")
                {
                    std::cerr << "Error: bad index directive" << std::endl;
                    return false;
                }
                current->locations.back().default_file = tokens[i + 1];
                i += 2;
            }
            else if (state == IN_LOCATION && tok == "upload")
            {
                if (i + 3 >= tokens.size() || tokens[i + 3] != ";")
                {
                    std::cerr << "Error: bad upload directive" << std::endl;
                    return false;
                }
                current->locations.back().upload_enabled = (tokens[i + 1] == "on");
                current->locations.back().upload_path = tokens[i + 2];
                i += 3;
            }
            else if (state == IN_LOCATION && tok == "cgi")
            {
                if (i + 3 >= tokens.size() || tokens[i + 3] != ";")
                {
                    std::cerr << "Error: bad cgi directive" << std::endl;
                    return false;
                }
                current->locations.back().cgi[tokens[i + 1]] = tokens[i + 2];
                i += 3;
            }
            else if (tok == "}")
            {
                if (state == IN_LOCATION)
                    state = IN_SERVER;
                else if (state == IN_SERVER)
                    state = GLOBAL;
                else
                {
                    std::cerr << "Error: unexpected '}'" << std::endl;
                    return false;
                }
            }
            else
            {
                std::cerr << "Error: unknown directive '" << tok << "'" << std::endl;
                return false;
            }
        }
    }

    if (state != GLOBAL)
    {
        std::cerr << "Error: unexpected end of config (missing '}')" << std::endl;
        return false;
    }
    if (servers.empty())
    {
        std::cerr << "Error: no server defined in config" << std::endl;
        return false;
    }
    return true;
}
