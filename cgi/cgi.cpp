#include "cgi.hpp"

char** env_pointer(std::vector<std::string> &env_vec)
{
    char** cgi_env = new char*[env_vec.size() + 1];

    for (int i = 0; i < env_vec.size(); i++)
        cgi_env[i] = strdup(env_vec[i].c_str());
    cgi_env[env_vec.size()] = NULL;
    return (cgi_env);
}

cgi_result exeute_cgi(HttpRequest& req, Server& ser, Location& loc, std::string client_ip)
{
    cgi_result cgi_res = {"", 0};

    std::string full_path = req.getPath();
    std::string ext = loc.cgi_extension;
    size_t pos = full_path.rfind(ext);
    std::string script_name, path_info;
    if (pos != std::string::npos)
    {
        script_name = full_path.substr(0, pos + ext.length());
        path_info = full_path.substr(pos + ext.length());
    }
    std::vector<std::string> env_vec = env_buider(req, script_name,
        ser.interface_ip, std::to_string(ser.port), client_ip, path_info);
    char **cgi_env = env_pointer(env_vec);
    std::string relative = script_name;
    if (relative.find(loc.path) == 0)
        relative = relative.substr(loc.path.length());
    std::string script_path = loc.root + relative;
    std::cout << "script_path = " << script_path << std::endl;
    char *argv[] = {
        strdup(loc.cgi_path.c_str()),
        strdup(script_path.c_str()),
        NULL
    };
    int in_pipe[2];
    int out_pipe[2];
    int saved_in, saved_out;
    pipe(in_pipe);
    pipe(out_pipe);
    int pid = fork();
    if (!pid)
    {
        dup2(in_pipe[0], 0);
        dup2(out_pipe[1], 1);
        close(in_pipe[1]);
        close(out_pipe[0]);
        execve(argv[0], argv, cgi_env);
    }
    else if (pid > 0)
    {
        close(in_pipe[0]);
        close(out_pipe[1]);
        if (req.getMethod() == "POST" || req.getMethod() == "PUT")
            write(in_pipe[1], req.getBody().c_str(), req.getBody().size());
        close(in_pipe[1]);
        char buffer[4096];
        size_t n;
        std::string raw_output;
        while ((n = read(out_pipe[0], buffer, sizeof(buffer))) > 0)
        {
            buffer[n] = 0;
            raw_output.append(buffer);
        }
        close(out_pipe[0]);
        int status;
        waitpid(pid, &status, 0);
        cgi_res.exit_status = WEXITSTATUS(status);
        cgi_res.raw_output = raw_output;
    }
    return (cgi_res);
}
