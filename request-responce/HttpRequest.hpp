#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <iostream>
#include <string>
#include <map>

enum ParseState
{
    REQUEST_LINE,
    HEADERS,
    BODY,
    COMPLETE,
    ERROR
};

enum ParseError
{
    NONE,
    ERR_BAD_REQUEST,
    ERR_METHOD_NOT_ALLOWED,
    ERR_VERSION_NOT_SUPPORTED
};

class HttpRequest
{
private:
    std::string _raw;

    std::string method;
    std::string uri;
    std::string query;
    std::string http_ver;

    std::map<std::string, std::string> headers;

    std::string body;
    size_t content_length;
    bool is_chunked;

    ParseState state;
    ParseError error;

    void parseRequestLine();
    void parseHeaders();
    void parseChunkedBody();

public:
    HttpRequest();

    void feed(const std::string &raw);

    bool isComplete() const;
    bool hasError() const;
    ParseError getError() const;

    std::string getMethod() const;
    std::string getPath() const;
    std::string getQuery() const;
    std::string getUri() const;
    std::string getHttpVersion() const;
    std::string getBody() const;
    size_t getContentLength() const;
    std::string getHeader(const std::string &key) const;
    std::map<std::string, std::string> getHeaders() const;
};

#endif
