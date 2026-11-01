#include <iostream>

#include "lcy/bytra/http/request_parser.hpp"

const char* request_parser_test_data[] =
    "GET / HTTP/1.1\r\n"
    "Host: example.com\r\n"
    "User-Agent: TestAgent/1.0\r\n"
    "\r\n";

void test_request_parser()
{
    lcy::bytra::http::request_parser parser;
    lcy::bytra::http::request request;
    lcy::system::error_code ec;

    parser.parse(request_parser_test_data[0], strlen(request_parser_test_data[0]), request, ec);

    if (ec) {
        std::cerr << "Error parsing request: " << ec.message() << std::endl;
        return;
    }

    std::cout << "Parsed request method: " << request.method() << std::endl;
    std::cout << "Parsed request target: " << request.target() << std::endl;
    std::cout << "Parsed request version: " << request.version() << std::endl;
    for (const auto& header : request.headers()) {
        std::cout << "Header: " << header.first << ": " << header.second << std::endl;
    }
}

int main()
{
    test_request_parser();

    return 0;
}