#include <iostream>
#include "cgi.hpp"

// adjust includes depending on your project

int main()
{
    // =========================
    // Fake HTTP Request
    // =========================

    HttpRequest req;

    std::string raw_request =
        "POST /test.py?id=42 HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 14\r\n"
        "\r\n"
        "name=marouane";

    // depending on your parser API
    req.feed(raw_request, raw_request);

    // =========================
    // Fake Server Config
    // =========================

    Server ser;

    ser.interface_ip = "127.0.0.1";
    ser.port = 8080;
    ser.root = "./";

    // =========================
    // Fake CGI Location
    // =========================

    Location loc;

    loc.path = "/";
    loc.root = "./";
    loc.cgi_extension = ".py";

    // IMPORTANT:
    // check your machine path:
    //
    // which python3
    //
    // common values:
    // /usr/bin/python3
    // /opt/homebrew/bin/python3

    loc.cgi_path = "/usr/bin/python3";

    // =========================
    // Execute CGI
    // =========================

    cgi_result result;
    try
    {
        result = exeute_cgi(req, ser, loc, "127.0.0.1");
    }
    catch (std::exception &e)
    {
        std::cerr << "CGI ERROR: "
                  << e.what()
                  << std::endl;
        return (1);
    }

    // =========================
    // Print Results
    // =========================

    std::cout << "========== CGI OUTPUT =========="
              << std::endl;

    std::cout << result.raw_output
              << std::endl;

    std::cout << "========== EXIT STATUS =========="
              << std::endl;

    std::cout << result.exit_status
              << std::endl;

    return (0);
}