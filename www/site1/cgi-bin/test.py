#!/usr/bin/env python3

import os

print("Content-Type: text/html\n")

print("<html>")
print("<body>")
print("<h1>CGI WORKS</h1>")

print("<h2>METHOD:", os.environ.get("REQUEST_METHOD"), "</h2>")
print("<h2>QUERY:", os.environ.get("QUERY_STRING"), "</h2>")

print("</body>")
print("</html>")