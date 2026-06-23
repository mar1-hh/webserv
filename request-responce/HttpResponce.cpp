#include "HttpResponce.hpp"
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

static std::string intToStr(long n)
{
    std::ostringstream oss;
    oss << n;
    return oss.str();
}

static std::string joinPath(const std::string &a, const std::string &b)
{
    if (a.empty())
        return b;
    if (b.empty())
        return a;
    bool slashA = (a[a.size() - 1] == '/');
    bool slashB = (b[0] == '/');
    if (slashA && slashB)
        return a + b.substr(1);
    if (!slashA && !slashB)
        return a + "/" + b;
    return a + b;
}

static std::string reasonPhrase(int code)
{
    switch (code)
    {
    case 200: return "OK";
    case 201: return "Created";
    case 204: return "No Content";
    case 301: return "Moved Permanently";
    case 400: return "Bad Request";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 413: return "Payload Too Large";
    case 500: return "Internal Server Error";
    case 501: return "Not Implemented";
    case 505: return "HTTP Version Not Supported";
    default: return "Error";
    }
}

static std::string guessType(const std::string &path)
{
    size_t dot = path.rfind('.');
    if (dot == std::string::npos)
        return "application/octet-stream";
    std::string ext = path.substr(dot);
    if (ext == ".html" || ext == ".htm") return "text/html";
    if (ext == ".css") return "text/css";
    if (ext == ".js") return "application/javascript";
    if (ext == ".json") return "application/json";
    if (ext == ".png") return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".gif") return "image/gif";
    if (ext == ".pdf") return "application/pdf";
    if (ext == ".txt") return "text/plain";
    return "application/octet-stream";
}

static bool readWholeFile(const std::string &path, std::string &out)
{
    int fd = open(path.c_str(), O_RDONLY);
    if (fd == -1)
        return false;
    char buf[8192];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0)
        out.append(buf, n);
    close(fd);
    return (n != -1);
}

HttpResponce::HttpResponce(HttpRequest &request, Server &serv, SessionManager &sm)
    : req(request), server(serv), sessions(sm),
      _located(false), contentType("text/html"), status_code(200), _is_cgi(false)
{
}

bool HttpResponce::matchLocation()
{
    const std::string &path = req.getPath();
    size_t best = 0;
    for (size_t i = 0; i < server.locations.size(); i++)
    {
        const std::string &lp = server.locations[i].path;
        if (path.compare(0, lp.size(), lp) == 0)
        {
            bool boundary = (lp == "/" || path.size() == lp.size() || path[lp.size()] == '/');
            if (boundary && lp.size() >= best)
            {
                best = lp.size();
                _location = server.locations[i];
                _located = true;
            }
        }
    }
    if (_located)
    {
        std::string base = _location.root.empty() ? server.root : _location.root;
        std::string remainder = path.substr(_location.path.size());
        real_path = joinPath(base, remainder);
    }
    return _located;
}

bool HttpResponce::methodAllowed() const
{
    if (_location.methods.empty())
        return true;
    for (size_t i = 0; i < _location.methods.size(); i++)
        if (_location.methods[i] == req.getMethod())
            return true;
    return false;
}

std::string HttpResponce::allowedMethods() const
{
    if (_location.methods.empty())
        return "GET, POST, DELETE";
    std::string out;
    for (size_t i = 0; i < _location.methods.size(); i++)
    {
        if (i)
            out += ", ";
        out += _location.methods[i];
    }
    return out;
}

bool HttpResponce::detectCgi()
{
    if (_location.cgi.empty())
        return false;

    const std::string &uri = req.getPath();
    std::map<std::string, std::string>::iterator it;
    for (it = _location.cgi.begin(); it != _location.cgi.end(); ++it)
    {
        const std::string &ext = it->first;
        size_t p = uri.find(ext);
        while (p != std::string::npos)
        {
            size_t after = p + ext.size();
            if (after == uri.size() || uri[after] == '/')
            {
                std::string script_name = uri.substr(0, after);
                std::string base = _location.root.empty() ? server.root : _location.root;
                std::string remainder = script_name.substr(_location.path.size());
                std::string fs = joinPath(base, remainder);

                struct stat sb;
                if (stat(fs.c_str(), &sb) == 0 && S_ISREG(sb.st_mode))
                {
                    _is_cgi = true;
                    _cgi_interp = it->second;
                    _cgi_script = fs;
                    _cgi_script_name = script_name;
                    _cgi_path_info = uri.substr(after);
                    return true;
                }
            }
            p = uri.find(ext, p + 1);
        }
    }
    return false;
}

void HttpResponce::serveFile(const std::string &fsPath)
{
    std::string content;
    if (!readWholeFile(fsPath, content))
    {
        status_code = 404;
        return;
    }
    fileContent = content;
    contentType = guessType(fsPath);
    status_code = 200;
}

void HttpResponce::listDirectory()
{
    DIR *dir = opendir(real_path.c_str());
    if (!dir)
    {
        status_code = 403;
        return;
    }
    std::string page = "<!DOCTYPE html><html><head><title>Index of ";
    page += req.getPath();
    page += "</title></head><body><h1>Index of " + req.getPath() + "</h1><ul>";

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        std::string name = entry->d_name;
        if (name == ".")
            continue;
        std::string href = joinPath(req.getPath(), name);
        page += "<li><a href=\"" + href + "\">" + name + "</a></li>";
    }
    page += "</ul></body></html>";
    closedir(dir);

    fileContent = page;
    contentType = "text/html";
    status_code = 200;
}

void HttpResponce::handleDir()
{
    if (!_location.default_file.empty())
    {
        std::string candidate = joinPath(real_path, _location.default_file);
        struct stat sb;
        if (stat(candidate.c_str(), &sb) == 0 && S_ISREG(sb.st_mode))
        {
            serveFile(candidate);
            return;
        }
    }
    if (_location.directory_listing)
        listDirectory();
    else
        status_code = 403;
}

void HttpResponce::handleGet()
{
    struct stat sb;
    if (stat(real_path.c_str(), &sb) == -1)
        status_code = 404;
    else if (S_ISDIR(sb.st_mode))
        handleDir();
    else if (S_ISREG(sb.st_mode))
        serveFile(real_path);
    else
        status_code = 404;
}

void HttpResponce::handlePost()
{
    if (!_location.upload_enabled)
    {
        handleGet();
        return;
    }

    std::string filename = req.getHeader("x-file-name");
    if (filename.empty())
    {
        std::string uri = req.getPath();
        size_t slash = uri.rfind('/');
        if (slash != std::string::npos && slash + 1 < uri.size())
        filename = uri.substr(slash + 1);
    }
    std::cout << filename << " here " << std::endl;
    if (filename.empty() || ("/"+filename) == (_location.path) || filename == _location.path)
        filename = "upload_" + intToStr(static_cast<long>(std::time(NULL)));
    
    std::string dest = joinPath(_location.upload_path, filename);
    int fd = open(dest.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if (fd == -1)
    {
        status_code = 500;
        return;
    }
    const std::string &body = req.getBody();
    if (!body.empty())
        write(fd, body.c_str(), body.size());
    close(fd);

    status_code = 200;
    fileContent = "<html><body><h1>200 OK</h1><p>Saved as " + filename + "</p></body></html>";
    contentType = "text/html";
}

void HttpResponce::handleDelete()
{
    struct stat sb;
    if (stat(real_path.c_str(), &sb) == -1)
        status_code = 404;
    else if (S_ISDIR(sb.st_mode))
        status_code = 403;
    else if (unlink(real_path.c_str()) == -1)
        status_code = 500;
    else
    {
        status_code = 200;
        fileContent = "<html><body><h1>200 OK</h1><p>Deleted</p></body></html>";
        contentType = "text/html";
    }
}

void HttpResponce::handleSession()
{
    std::string cookie = req.getHeader("cookie");
    std::string id;
    size_t pos = cookie.find("session_id=");
    if (pos != std::string::npos)
    {
        size_t start = pos + 11;
        size_t end = cookie.find(';', start);
        id = cookie.substr(start, (end == std::string::npos ? cookie.size() : end) - start);
    }

    bool fresh = false;
    if (id.empty() || !sessions.has(id))
    {
        id = sessions.create();
        fresh = true;
    }

    std::map<std::string, std::string> &data = sessions.data(id);
    long visits = std::atol(data["visits"].c_str()) + 1;
    data["visits"] = intToStr(visits);

    if (fresh)
        extra_headers.push_back("Set-Cookie: session_id=" + id + "; Path=/");

    fileContent = "<!DOCTYPE html><html><body><h1>Session demo</h1>";
    fileContent += "<p>Session id: " + id + "</p>";
    fileContent += "<p>Visits in this session: " + intToStr(visits) + "</p>";
    fileContent += "</body></html>";
    contentType = "text/html";
    status_code = 200;
}

void HttpResponce::process()
{
    if (req.hasError())
    {
        if (req.getError() == ERR_VERSION_NOT_SUPPORTED)
            status_code = 505;
        else if (req.getError() == ERR_METHOD_NOT_ALLOWED)
            status_code = 501;
        else
            status_code = 400;
        build();
        return;
    }
    else if (req.getBody().length() > server.max_body_size)
    {
        status_code = 413;
        build();
        return;
    }

    if (req.getPath() == "/session")
    {
        handleSession();
        build();
        return;
    }

    if (!matchLocation())
    {
        status_code = 404;
        build();
        return;
    }

    if (!_location.redirection.empty())
    {
        status_code = 301;
        extra_headers.push_back("Location: " + _location.redirection);
        build();
        return;
    }

    if (!methodAllowed())
    {
        status_code = 405;
        extra_headers.push_back("Allow: " + allowedMethods());
        build();
        return;
    }

    if (detectCgi())
        return;

    const std::string &method = req.getMethod();
    if (method == "GET" || method == "HEAD")
        handleGet();
    else if (method == "POST")
        handlePost();
    else if (method == "DELETE")
        handleDelete();
    else
        status_code = 501;

    build();
}

void HttpResponce::buildErrorBody()
{
    std::map<int, std::string>::iterator it = server.error_pages.find(status_code);
    if (it != server.error_pages.end())
    {
        std::string page;
        if (readWholeFile(joinPath(server.root, it->second), page))
        {
            fileContent = page;
            contentType = "text/html";
            return;
        }
    }
    fileContent = "<!DOCTYPE html><html><head><title>" + intToStr(status_code) +
                  "</title></head><body><h1>" + intToStr(status_code) + " " +
                  reasonPhrase(status_code) + "</h1></body></html>";
    contentType = "text/html";
}

void HttpResponce::build()
{
    if (status_code >= 400)
        buildErrorBody();

    std::string ver = req.getHttpVersion().empty() ? "HTTP/1.1" : req.getHttpVersion();

    // ADDED: Date and Server headers to pass the response-headers test
    char datebuf[64];
    time_t now = std::time(NULL);
    struct tm *gmt = gmtime(&now);
    strftime(datebuf, sizeof(datebuf), "%a, %d %b %Y %H:%M:%S GMT", gmt);

    std::ostringstream out;
    out << ver << " " << status_code << " " << reasonPhrase(status_code) << "\r\n";
    out << "Date: " << datebuf << "\r\n";
    out << "Server: webserv/1.0\r\n";
    out << "Content-Type: " << contentType << "\r\n";
    out << "Content-Length: " << fileContent.size() << "\r\n";
    out << "Connection: close\r\n";
    for (size_t i = 0; i < extra_headers.size(); i++)
        out << extra_headers[i] << "\r\n";
    out << "\r\n";
    if (req.getMethod() != "HEAD")
        out << fileContent;
    responce = out.str();
}

bool HttpResponce::isCgi() const { return _is_cgi; }
const std::string &HttpResponce::cgiInterpreter() const { return _cgi_interp; }
const std::string &HttpResponce::cgiScript() const { return _cgi_script; }
const std::string &HttpResponce::cgiScriptName() const { return _cgi_script_name; }
const std::string &HttpResponce::cgiPathInfo() const { return _cgi_path_info; }
std::string HttpResponce::getResponce() const { return responce; }
