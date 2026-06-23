# Usage

How to build, configure and use the server.

## Build

On Linux or macOS, with the provided Makefile:

```
make        # build ./webserv
make clean  # remove object files
make fclean # remove objects + binary
make re     # rebuild from scratch
```

The build uses `c++ -Wall -Wextra -Werror -std=c++98`.

## Run

```
./webserv                # uses ./config.conf
./webserv path/to.conf   # uses the given configuration file
```

The server prints the address/port of every socket it manages to listen on
and then runs until you stop it with `Ctrl-C`.

## Configuration file

The file is a list of `server { ... }` blocks. Each server can contain
several `location { ... }` blocks. Directives end with `;`.

### Server directives

| Directive        | Example                          | Meaning                              |
|------------------|----------------------------------|--------------------------------------|
| `host`           | `host 0.0.0.0;`                  | Interface to bind (`interface` works too) |
| `listen`         | `listen 8080;`                   | Port to listen on                    |
| `root`           | `root ./www/site1;`              | Default document root for the server |
| `max_body_size`  | `max_body_size 10000000;`        | Max client body in bytes (0 = unlimited) |
| `error_page`     | `error_page 404 /errors/404.html;` | Custom page for a status code      |

### Location directives

A location matches by longest URI prefix. Inside a location:

| Directive            | Example                         | Meaning                                   |
|----------------------|---------------------------------|-------------------------------------------|
| `methods`            | `methods GET POST DELETE;`      | Allowed methods (others get 405)          |
| `root`               | `root ./www/site1/images;`      | Override the server root for this route   |
| `index`              | `index index.html;`             | File served when the URI is a directory   |
| `directory_listing`  | `directory_listing on;`         | Auto-generate a listing (`autoindex` works too) |
| `redirection`        | `redirection http://example.com;` | Reply 301 to the given URL              |
| `upload`             | `upload on ./www/site1/uploads;` | Enable uploads and set the storage path  |
| `cgi`                | `cgi .py /usr/bin/python3;`     | Run files with this extension through the interpreter |

The root works like NGINX: with `location /kapouet { root /tmp/www; }`, the URL
`/kapouet/pouic/toto` is served from `/tmp/www/pouic/toto`.

Several `cgi` lines can be set in one location to support multiple types, e.g.
`cgi .py /usr/bin/python3;` and `cgi .sh /bin/bash;`.

## What the shipped config exposes

Server 1 — `http://localhost:8080` (root `./www/site1`):

- `/` — static site, `index.html` (GET)
- `/list/` — directory listing
- `/images/` — directory listing of images
- `/uploads/` — upload / list / delete files (GET POST DELETE)
- `/redirect` — 301 to `http://example.com`
- `/cgi-bin/test.py`, `/cgi-bin/hello.sh` — CGI scripts

Server 2 — `http://127.0.0.1:9090` (root `./www/site2`):

- `/`, `/home` — static site, `home.html` (GET POST)
- `/list/` — directory listing
- `/uploads/` — upload / list / delete files
- `/redirect` — 301 to `http://example.com`

`/session` works on any server (see Sessions below).

## Examples

Static file and listing:

```
curl http://localhost:8080/
curl http://localhost:8080/list/
```

Upload, then download, then delete a file:

```
curl -X POST --data-binary @notes.txt http://localhost:8080/uploads/notes.txt
curl http://localhost:8080/uploads/notes.txt
curl -X DELETE http://localhost:8080/uploads/notes.txt
```

CGI (GET with a query string, and POST with a body):

```
curl "http://localhost:8080/cgi-bin/test.py?name=alice"
curl -X POST --data "x=10&y=20" http://localhost:8080/cgi-bin/test.py
curl "http://localhost:8080/cgi-bin/hello.sh"
```

Redirection:

```
curl -i http://localhost:8080/redirect
```

## Sessions and cookies (bonus)

Request `/session` to get a small page that counts your visits. On the first
visit the server sends a `Set-Cookie: session_id=...` header and keeps an
in-memory counter for that id; sending the cookie back increments it:

```
curl -i http://localhost:8080/session                 # gets a session_id cookie
curl -i --cookie "session_id=<value>" http://localhost:8080/session
```

A browser does this automatically, so just reload the page to see the counter
go up.

## CGI notes

- The script is chosen by file extension inside a `cgi`-enabled location.
- The request body is written to the script's stdin; for chunked requests the
  body is un-chunked first.
- The script's output is read from stdout. A `Status:` header sets the HTTP
  status, other headers (like `Content-Type`) are forwarded, and if no
  `Content-Length` is given the end of output marks the end of the body.
- The script runs from its own directory so relative paths work.
- Make sure the interpreter paths in the config exist on your machine
  (`which python3`, `which bash`) and that the scripts are readable.
