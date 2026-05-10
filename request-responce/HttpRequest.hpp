#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP
#include <iostream>
#include <exception>
#include <map>

typedef enum ParseState
{
    REQUEST_LINE,
    HEADERS,
    BODY,
    COMPLETE,
    ERROR
}t_parseState;

typedef enum ParseError
{
    NONE,
    ERR_BAD_REQUEST,           // 400
    ERR_METHOD_NOT_ALLOWED,    // 405
    ERR_VERSION_NOT_SUPPORTED, // 505
    ERR_CONTENT_TOO_LARGE,     // 413
    ERR_URI_TOO_LONG           // 414
} t_parseError;

typedef struct s_state{
    t_parseState _state;
    t_parseError _error;
}t_state;

class HttpRequest{
    private:
        std::string _raw;
        //lines 
        std::string method;
        std::string uri;
        std::string query;
        std::string path;
        std::string http_ver;
        
        //headers 
        std::map<std::string, std::string> headers;
        //body
        std::string body;
        int content_length;
        bool is_chuncked;
        std::string transfer_encoding;

        //state
        t_state _state;


    public:
        //exepetions
        class MethodNotAllowed : public std::exception
        {
            public:
                virtual const char *what() const throw();
        };
        class VersionNotSupported : public std::exception
        {
            public:
                virtual const char *what() const throw();
        };

        HttpRequest(void);
        //parsing 
        void feed(std::string raw, std::string buffer);
        bool isComplete() const;
        bool hasError() const;
        t_parseError getError() const;

        //internal parsing
        void parseRequestLine();
        void parseHeaders();
        void parseBody(const std::string &data);
        void parseChunkedBody(const std::string &data);

        // ── Validation ────────────────────────────
        bool isMethodValid() const;
        bool isVersionValid() const;
        bool isBodyExpected() const; // true if POST/PUT

        // ── Getters ───────────────────────────────
        std::string getMethod() const;
        std::string getPath() const;
        std::string getQuery() const;
        std::string getUri() const;
        std::string getHttpVersion() const;
        std::string getBody() const;
        size_t getContentLength() const;
        std::string getHeader(const std::string &key) const;
        std::map<std::string, std::string> getHeaders() const;
        t_parseState getState() const;

        
};
#endif