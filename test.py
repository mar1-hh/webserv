import requests

ip = "127.0.0.1"
port = 8080
BASE = f"http://{ip}:{port}"

passed = 0
failed = 0

def test(name, method, path, params=None, data=None, headers=None, expected_status=None):
    global passed, failed
    url = BASE + path
    try:
        if method == "GET":
            res = requests.get(url, params=params, headers=headers, timeout=5)
        elif method == "POST":
            res = requests.post(url, params=params, data=data, headers=headers, timeout=5)
        elif method == "DELETE":
            res = requests.delete(url, params=params, headers=headers, timeout=5)

        status_ok = (expected_status is None) or (res.status_code == expected_status)
        symbol = "✅" if status_ok else "❌"
        if status_ok:
            passed += 1
        else:
            failed += 1

        print(f"{symbol} [{method}] {path}")
        print(f"   status  : {res.status_code}")
        if params:
            print(f"   params  : {params}")
        if data:
            print(f"   body    : {data}")
        print(f"   response: {res.text[:100]}")
        print()

    except requests.exceptions.ConnectionError:
        failed += 1
        print(f"💀 [{method}] {path} → connection refused (server down?)\n")
    except requests.exceptions.Timeout:
        failed += 1
        print(f"⏱️  [{method}] {path} → timeout\n")


print("=" * 50)
print("        WEBSERV TEST SUITE")
print("=" * 50 + "\n")

# ── GET requests ──────────────────────────────────────
test("GET index",           "GET",  "/",                                            expected_status=200)
test("GET static file",     "GET",  "/index.html",                                  expected_status=200)
test("GET with query",      "GET",  "/search",      params={"q": "hello"},          expected_status=200)
test("GET multi query",     "GET",  "/filter",      params={"page": "1", "limit": "10", "sort": "asc"}, expected_status=200)
test("GET not found",       "GET",  "/notexist",                                    expected_status=404)
test("GET deep path",       "GET",  "/api/users/42",                                expected_status=200)

# ── POST requests ─────────────────────────────────────
test("POST form body",      "POST", "/submit",
     data={"username": "jawad", "password": "1337"},                                expected_status=200)

test("POST with query+body","POST", "/submit",
     params={"source": "web"},
     data={"username": "ali", "email": "ali@1337.ma"},                              expected_status=200)

test("POST large body",     "POST", "/upload",
     data={"file_content": "A" * 5000, "filename": "test.txt"},                     expected_status=200)

test("POST empty body",     "POST", "/submit",
     data={},                                                                        expected_status=400)

test("POST json-like body", "POST", "/api/create",
     data={"name": "webserv", "version": "1.0", "lang": "cpp"},                     expected_status=200)

test("POST custom header",  "POST", "/submit",
     data={"msg": "hello"},
     headers={"X-Custom-Header": "42network"},                                       expected_status=200)

# ── DELETE requests ───────────────────────────────────
test("DELETE resource",     "DELETE", "/files/test.txt",                            expected_status=200)
test("DELETE not found",    "DELETE", "/files/ghost.txt",                           expected_status=404)
test("DELETE with query",   "DELETE", "/files/old.txt", params={"confirm": "true"}, expected_status=200)

# ── Edge cases ────────────────────────────────────────
test("GET double slash",    "GET",  "//index.html",                                 expected_status=200)
test("GET empty path",      "GET",  "/",                                            expected_status=200)
test("GET special chars",   "GET",  "/search",      params={"q": "hello world!"},   expected_status=200)
test("GET encoded uri",     "GET",  "/path%20with%20spaces",                        expected_status=404)

# ── Summary ───────────────────────────────────────────
print("=" * 50)
print(f"  Results: {passed} passed  |  {failed} failed")
print("=" * 50)