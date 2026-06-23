*This project has been created as part of the 42 curriculum by &lt;login&gt;.*

## Description

Webserv is a small HTTP/1.1 server written in C++98. It reads a configuration
file (inspired by the NGINX `server` block), listens on one or more
`host:port` pairs and serves static content, handles file uploads, runs CGI
scripts and answers with accurate HTTP status codes.

The whole program runs on a single non-blocking event loop built around
`poll()`. Every socket and every CGI pipe is set non-blocking and is only read
from or written to after `poll()` reports it ready, so the server never blocks
on a slow or misbehaving client and stays available under load.

Implemented features:

- Configuration file with multiple servers, `host:port`, `root`, custom error
  pages and a maximum client body size.
- Per-route rules: allowed methods, HTTP redirection, alternate root,
  directory listing, default index file, file uploads and CGI.
- `GET`, `POST` and `DELETE` methods (plus `HEAD`).
- Static file serving, directory listing and default error pages.
- CGI execution chosen by file extension, with the request body passed on
  stdin and the response read from stdout.
- **Bonus:** cookie based sessions (see `/session`) and several CGI types
  (`.py` and `.sh`) configured per route.

## Instructions

Build with the provided Makefile (Linux / macOS, `c++` with
`-Wall -Wextra -Werror -std=c++98`):

```
make
```

Run the server, optionally passing a configuration file (defaults to
`config.conf`):

```
./webserv
./webserv config.conf
```

Then open a browser or use `curl`:

```
curl http://localhost:8080/                 # static page
curl http://localhost:8080/list/            # directory listing
curl http://localhost:8080/cgi-bin/test.py  # CGI (python)
curl -X POST --data-binary @file.txt http://localhost:8080/uploads/file.txt
curl http://localhost:8080/session          # session / cookie demo
```

`make clean` removes the object files, `make fclean` also removes the binary,
and `make re` rebuilds from scratch.

## Resources

- RFC 7230 / RFC 7231 — HTTP/1.1 message syntax and semantics.
- RFC 3875 — The Common Gateway Interface (CGI/1.1).
- Beej's Guide to Network Programming (sockets, `poll`).
- The NGINX configuration documentation, used as inspiration for the config
  file syntax and for comparing response headers.
- `man` pages for `poll`, `socket`, `bind`, `recv`, `send`, `fork`, `execve`.

### How AI was used

AI was used as a reviewer and debugging aid: to cross-check our implementation
against the subject requirements (the non-blocking `poll()` rule, allowed
functions, README rules), to help track down bugs in the event loop and the
request parser, and to speed up repetitive boilerplate. Every suggestion was
read, tested and adapted by us before being kept.
