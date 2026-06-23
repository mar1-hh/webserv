import requests

passed = 0
failed = 0

def test(name, method, path, port=8080, params=None, data=None, headers=None, expected_status=None, allow_redirects=True):
    global passed, failed
    BASE = f"http://127.0.0.1:{port}"
    url = BASE + path
    try:
        if method == "GET":
            res = requests.get(url, params=params, headers=headers, timeout=5, allow_redirects=allow_redirects)
        elif method == "POST":
            res = requests.post(url, params=params, data=data, headers=headers, timeout=5, allow_redirects=allow_redirects)
        elif method == "DELETE":
            res = requests.delete(url, params=params, headers=headers, timeout=5, allow_redirects=allow_redirects)

        status_ok = (expected_status is None) or (res.status_code == expected_status)
        symbol = "✅" if status_ok else "❌"
        if status_ok:
            passed += 1
        else:
            failed += 1

        print(f"{symbol} [{method}] :{port}{path} → {name}")
        print(f"   status  : {res.status_code} (expected {expected_status})")
        if params:
            print(f"   params  : {params}")
        if data:
            print(f"   body    : {str(data)[:80]}")
        print(f"   response: {res.text[:100]}")
        print()

    except requests.exceptions.ConnectionError:
        failed += 1
        print(f"💀 [{method}] :{port}{path} → connection refused (server down?)\n")
    except requests.exceptions.Timeout:
        failed += 1
        print(f"⏱️  [{method}] :{port}{path} → timeout\n")


print("=" * 55)
print("            WEBSERV TEST SUITE")
print("=" * 55 + "\n")

# ──────────────────────────────────────────────────────
# SERVER 1 — port 8080 — root ./www/site1
# locations: /  (GET only)
#            /images (GET only, autoindex on)
# max_body_size: 1000000
# ──────────────────────────────────────────────────────
print("── SERVER 1 (port 8080) ──────────────────────────────\n")

# location / → GET allowed
test("GET index page",          "GET",  "/",            port=8080, expected_status=200)
test("GET index.html directly", "GET",  "/index.html",  port=8080, expected_status=200)
test("GET with query string",   "GET",  "/",            port=8080,
     params={"lang": "cpp", "version": "98"},           expected_status=200)

# location /images → GET allowed, autoindex on
test("GET images autoindex",    "GET",  "/images",      port=8080, expected_status=200)
test("GET image file",          "GET",  "/images/logo.png", port=8080, expected_status=200)

# method not allowed on server1
test("POST on / → 405",         "POST", "/",            port=8080,
     data={"key": "value"},                             expected_status=405)
test("DELETE on / → 405",       "DELETE", "/",          port=8080, expected_status=405)
test("POST on /images → 405",   "POST", "/images",      port=8080,
     data={"key": "value"},                             expected_status=405)

# no matching location
test("GET unknown path → 404",  "GET",  "/notexist",    port=8080, expected_status=404)
test("GET 404 error page",      "GET",  "/missing.html",port=8080, expected_status=404)

# max_body_size = 1000000 — send just under and just over
test("POST body under limit",   "POST", "/",            port=8080,
     data={"data": "A" * 999990},                       expected_status=405) # POST not allowed anyway
test("GET with long query",     "GET",  "/",            port=8080,
     params={"q": "x" * 500},                           expected_status=200)


# ──────────────────────────────────────────────────────
# SERVER 2 — port 9090 — root ./www/site2
# locations: /         (GET POST)
#            /upload   (POST only, upload enabled)
#            /redirect (301 → http://example.com)
# max_body_size: 2000000
# ──────────────────────────────────────────────────────
print("── SERVER 2 (port 9090) ──────────────────────────────\n")

# location / → GET and POST allowed
test("GET home page",           "GET",  "/",            port=9090, expected_status=200)
test("GET home.html directly",  "GET",  "/home.html",   port=9090, expected_status=200)
test("POST to /",               "POST", "/",            port=9090,
     data={"username": "jawad", "password": "1337"},    expected_status=200)
test("POST with query + body",  "POST", "/",            port=9090,
     params={"source": "test"},
     data={"name": "42network"},                        expected_status=200)

# DELETE not allowed on /
test("DELETE on / → 405",       "DELETE", "/",          port=9090, expected_status=405)

# location /upload → POST only
test("POST upload small file",  "POST", "/upload",      port=9090,
     data={"file_content": "hello world", "filename": "test.txt"}, expected_status=200)
test("POST upload large body",  "POST", "/upload",      port=9090,
     data={"file_content": "B" * 1000000},              expected_status=200)
test("POST upload over limit",  "POST", "/upload",      port=9090,
     data={"file_content": "C" * 2000001},              expected_status=413) # too large
test("GET on /upload → 405",    "GET",  "/upload",      port=9090, expected_status=405)
test("DELETE on /upload → 405", "DELETE", "/upload",    port=9090, expected_status=405)

# location /redirect → 301
test("GET redirect → 301",      "GET",  "/redirect",    port=9090, expected_status=301, allow_redirects=False)
test("POST redirect → 301",     "POST", "/redirect",    port=9090,
     data={"x": "y"},                                   expected_status=301, allow_redirects=False)

# no matching location
test("GET unknown → 404",       "GET",  "/ghost",       port=9090, expected_status=404)
test("GET deep unknown → 404",  "GET",  "/api/v1/test", port=9090, expected_status=404)


# ──────────────────────────────────────────────────────
print("=" * 55)
print(f"  Results: {passed} passed  |  {failed} failed")
print("=" * 55)