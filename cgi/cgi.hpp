#ifndef CGI_HPP
#define CGI_HPP

#include "../request-responce/HttpRequest.hpp"
#include <string>
#include <vector>

std::vector<std::string> build_cgi_env(HttpRequest &req,
                                       const std::string &script_name,
                                       const std::string &script_filename,
                                       const std::string &path_info,
                                       const std::string &server_name,
                                       const std::string &server_port,
                                       const std::string &client_ip);

#endif
