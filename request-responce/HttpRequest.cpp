#include "HttpRequest.hpp"
#include <sstream>
#include <cstdlib>
#include <cctype>

static std::string toLower(const std::string &s)
{
    std::string out = s;
    for (size_t i = 0; i < out.size(); i++)
        out[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(out[i])));
    return out;
}

HttpRequest::HttpRequest()
    : content_length(0), is_chunked(false), state(REQUEST_LINE), error(NONE)
{
}

void HttpRequest::parseRequestLine()
{
    size_t eol = _raw.find("\r\n");
    std::string line = (eol == std::string::npos) ? _raw : _raw.substr(0, eol);

    size_t sp1 = line.find(' ');
    if (sp1 == std::string::npos)
    {
        state = ERROR;
        error = ERR_BAD_REQUEST;
        return;
    }
    method = line.substr(0, sp1);

    size_t sp2 = line.find(' ', sp1 + 1);
    if (sp2 == std::string::npos)
    {
        state = ERROR;
        error = ERR_BAD_REQUEST;
        return;
    }
    std::string target = line.substr(sp1 + 1, sp2 - sp1 - 1);
    std::string version = line.substr(sp2 + 1);

    if (method != "GET" && method != "POST" && method != "DELETE" &&
        method != "PUT" && method != "HEAD" && method != "OPTIONS")
    {
        state = ERROR;
        error = ERR_METHOD_NOT_ALLOWED;
        return;
    }

    size_t q = target.find('?');
    if (q == std::string::npos)
    {
        uri = target;
        query = "";
    }
    else
    {
        uri = target.substr(0, q);
        query = target.substr(q + 1);
    }

    if (version == "HTTP/1.1" || version == "HTTP/1.0")
        http_ver = version;
    else
    {
        state = ERROR;
        error = ERR_VERSION_NOT_SUPPORTED;
    }
}

void HttpRequest::parseHeaders()
{
    size_t start = _raw.find("\r\n");
    if (start == std::string::npos)
        return;
    start += 2;

    while (start < _raw.size())
    {
        size_t eol = _raw.find("\r\n", start);
        if (eol == std::string::npos || eol == start)
            break;

        std::string field = _raw.substr(start, eol - start);
        start = eol + 2;

        size_t colon = field.find(':');
        if (colon == std::string::npos)
            continue;

        std::string key = toLower(field.substr(0, colon));
        std::string value = field.substr(colon + 1);
        size_t first = value.find_first_not_of(" \t");
        value = (first == std::string::npos) ? "" : value.substr(first);
        headers[key] = value;
    }
}

void HttpRequest::parseChunkedBody()
{
    size_t pos = _raw.find("\r\n\r\n");
    if (pos == std::string::npos)
        return;
    std::string str = _raw.substr(pos + 4);

    while (true)
    {
        size_t eol = str.find("\r\n");
        if (eol == std::string::npos)
            break;

        std::istringstream iss(str.substr(0, eol));
        size_t chunk_size = 0;
        iss >> std::hex >> chunk_size;
        if (chunk_size == 0)
            break;

        str = str.substr(eol + 2);
        if (str.size() < chunk_size)
            break;
        body += str.substr(0, chunk_size);
        str = (str.size() >= chunk_size + 2) ? str.substr(chunk_size + 2) : "";
    }
    content_length = body.length();
}

void HttpRequest::feed(const std::string &raw)
{
    _raw = raw;

    parseRequestLine();
    if (state == ERROR)
        return;

    parseHeaders();

    std::map<std::string, std::string>::iterator te = headers.find("transfer-encoding");
    if (te != headers.end() && te->second == "chunked")
    {
        is_chunked = true;
        parseChunkedBody();
        state = COMPLETE;
        return;
    }

    std::map<std::string, std::string>::iterator cl = headers.find("content-length");
    if (cl != headers.end())
    {
        content_length = static_cast<size_t>(std::atol(cl->second.c_str()));
        size_t header_end = _raw.find("\r\n\r\n");
        if (header_end != std::string::npos)
            body = _raw.substr(header_end + 4, content_length);
    }
    state = COMPLETE;
}

std::string HttpRequest::getMethod() const { return method; }
std::string HttpRequest::getUri() const { return uri; }
std::string HttpRequest::getPath() const { return uri; }
std::string HttpRequest::getHttpVersion() const { return http_ver; }
std::string HttpRequest::getQuery() const { return query; }
std::string HttpRequest::getBody() const { return body; }
std::map<std::string, std::string> HttpRequest::getHeaders() const { return headers; }
size_t HttpRequest::getContentLength() const { return content_length; }
bool HttpRequest::isComplete() const { return state == COMPLETE; }
bool HttpRequest::hasError() const { return state == ERROR; }
ParseError HttpRequest::getError() const { return error; }

std::string HttpRequest::getHeader(const std::string &key) const
{
    std::map<std::string, std::string>::const_iterator it = headers.find(toLower(key));
    if (it == headers.end())
        return "";
    return it->second;
}
