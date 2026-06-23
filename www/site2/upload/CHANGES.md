# Changes

Summary of the work done to bring the server in line with the subject
(mandatory part + bonus). Grouped by area, with the reason for each change.

## Networking / event loop (`server_core`)

- Rewrote the core into a single non-blocking `poll()` loop. Every socket and
  every CGI pipe is set non-blocking with `fcntl(fd, F_SETFL, O_NONBLOCK)` and
  is only read from or written to after `poll()` reports it ready.
  *Why:* the subject requires one `poll()` for all I/O and forbids reading or
  writing without prior readiness; the previous code sent on blocking sockets
  directly.
- Each connection now has its own input and output buffers and a small state
  machine (`reading` → `cgi` / `writing`). `poll()` watches `POLLOUT` while a
  response is being sent.
  *Why:* responses must be sent through `poll()`, and partial reads/writes must
  be handled across loop iterations.
- Added `setsockopt(SO_REUSEADDR)` and return-value checks on
  `socket`/`bind`/`listen`; a failing server is skipped with an error message
  instead of running on a broken socket.
- `host` from the config is parsed and bound (instead of always `INADDR_ANY`).
  The dotted-quad is parsed by hand to avoid `inet_addr`/`inet_ntoa`, which are
  not in the allowed-functions list.
- Removed the erase-while-iterating bug: the loop now iterates over a snapshot
  and adds/removes pollfds safely.
- Added timeouts so a request never hangs forever: idle reading clients and
  overrunning CGI processes are dropped (CGI returns `504`).
- `errno` is never inspected after `recv`/`send`; the byte count drives the
  logic, as required.
- `SIGPIPE` is ignored in `main` so a write to a closed socket cannot kill the
  process.

## Command line (`server.cpp`)

- `main` now takes the configuration file from `argv[1]` (falling back to
  `config.conf`), matching `./webserv [configuration file]`.

## CGI

- CGI is now executed and driven by the same `poll()` loop: the body is streamed
  to the script's stdin via `POLLOUT`, output is read from stdout via `POLLIN`,
  and the child is reaped with `waitpid`. The old version read the pipe with a
  blocking loop.
- The script runs from its own directory (`chdir`) so relative paths work.
- The environment is built in `cgi/env_builder.cpp` (REQUEST_METHOD,
  QUERY_STRING, CONTENT_LENGTH/TYPE, PATH_INFO, SCRIPT_NAME, SERVER_*,
  REMOTE_ADDR, plus `HTTP_*` for each request header).
- CGI output is parsed for a `Status:` line and headers before the response is
  assembled.
- Replaced the `std::to_string(...)` call (C++11) so the module builds under
  `-std=c++98`.
- Removed the standalone `cgi/main.cpp` test harness (it had its own `main`) and
  the unused blocking `cgi/cgi.cpp`.

## Request parsing (`HttpRequest`)

- Made the parser crash-safe: every `substr`/`find` checks for `npos` first, so
  a malformed request line cannot throw.
- Header names are stored lower-cased and looked up case-insensitively (HTTP
  headers are case-insensitive).
- `feed` now takes the complete request; the server decides when a request is
  complete (Content-Length aware, and waits for the terminator on chunked
  bodies) before parsing.
- Removed the decorative debug printing and unused members.

## Response building (`HttpResponce`)

- Per-location `root` is now honoured (NGINX-style rooting), not just the server
  root.
- Uploads are written to the location's `upload_path` with a filename derived
  from the URI, instead of opening the request path directly.
- Directory listing uses the resolved path and builds clean links (no `//`).
- The response echoes the request's HTTP version, sends `Connection: close`,
  adds an `Allow` header on `405`, and serves the configured `error_page` (or a
  default page) for errors.
- CGI detection happens during routing; when a request maps to a CGI the
  response object hands the script/interpreter/path info to the event loop.

## Configuration parser

- Accepts `host` (and `interface`), and `autoindex` as an alias of
  `directory_listing`, so the config no longer drops directives silently.
- `cgi` can be specified several times per location (extension → interpreter)
  to support multiple CGI types.
- Unknown directives now produce an error instead of being ignored, and the
  parser reports a clear message and fails on a bad file.

## Config file (`config.conf`)

- Removed the duplicated `location /home`.
- Pointed `upload` and `/cgi-bin` at real directories under `www/`, and added a
  second CGI type (`.sh`).

## Build, docs, hygiene

- `Makefile`: enabled `-Wall -Wextra -Werror -std=c++98`, renamed the binary to
  `webserv`, and listed the CGI source. A second `make` does not relink.
- Wrote a proper `README.md` (description, instructions, resources, and how AI
  was used) and this changelog plus `USAGE.md`.
- Added the bonus features: cookie-based sessions (`/session`) and multiple CGI
  types.

## Notes / things to check

- The project was edited on a machine without a C++ compiler, so it has **not**
  been compiled or run here — build and test it on the 42 setup.
- `description.txt` describes the older design and is now out of date.
- The root `test.py` was written against a different set of routes than the
  current `config.conf`; update it before relying on it.
