#include <iostream>

#include "lcy/bytra/flat_buffer.hpp"
#include "lcy/bytra/http/request.hpp"

void test_request()
{
    lcy::bytra::http::request<lcy::bytra::flat_buffer> request;
    std::cout << "header_to_string : \n" << request.header_to_string() << std::endl;

    request.set_header("aaaa", "bbbbb");
    request.set_header("cccc", "ddddd");
    request.set_header("eeee", "mmmmm");
    std::cout << "add more header, header_to_string : \n" << request.header_to_string() << std::endl;

    request.set_path("/abcdefg/hijklmn");
    request.set_version(lcy::bytra::http::version::http_1_0);
    std::cout << "add path and version, header_to_string : \n" << request.header_to_string() << std::endl;

    auto buffer = request.body_container().prepare(3);
    ::memcpy(buffer.data(), "ok", 3);
    std::cout << "add body : \n" << request.header_to_string() + std::string((char*)buffer.data(), buffer.length()) << std::endl;
}

int main()
{
    test_request();

    return 0;
}