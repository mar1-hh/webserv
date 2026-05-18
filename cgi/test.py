#!/usr/bin/python3

import os
import sys

body = sys.stdin.read()

print("Content-Type: text/html")
print()

print("<h1>CGI Works</h1>")
print("<p>Method:", os.environ.get("REQUEST_METHOD"), "</p>")
print("<p>Query:", os.environ.get("QUERY_STRING"), "</p>")
print("<p>Body:", body, "</p>")