// test_tcp_resolver.cc
#include <cassert>
#include <iostream>
#include <list>
#include <string>

#include "lcy/asio/ip/tcp.hpp"
#include "lcy/asio/ip/udp.hpp"

using namespace lcy::asio;

static void test_resolve_host_service()
{
    std::cout << "[test_resolve_host_service]\n";

    ip::tcp::resolver resolver;
    std::list<ip::tcp::endpoint> endpoints;

    resolver.resolve("www.baidu.com", "http", ip::tcp::v4(), endpoints);

    assert(!endpoints.empty());

    for ( auto& ep : endpoints ) {
        assert(ep.is_v4());
        assert(ep.family() == AF_INET);
        std::string s = ep.to_string();
        assert(s.find(":80") != std::string::npos);
        std::cout << "  " << s << "\n";
    }

    std::cout << "  OK, " << endpoints.size() << " endpoints\n";
}

static void test_resolve_ip_port()
{
    std::cout << "[test_resolve_ip_port]\n";

    ip::tcp::resolver resolver;
    std::list<ip::tcp::endpoint> endpoints;

    resolver.resolve("127.0.0.1", "8080", ip::tcp::v4(), endpoints);

    assert(!endpoints.empty());
    assert(endpoints.front().to_string() == "127.0.0.1:8080");
    std::cout << "  " << endpoints.front().to_string() << "\n";
}

static void test_resolve_fail()
{
    std::cout << "[test_resolve_fail]\n";

    ip::tcp::resolver resolver;
    std::list<ip::tcp::endpoint> endpoints;

    lcy::system::error_code ec;
    resolver.resolve("this.domain.should.not.exist.invalid",
        "http", ip::tcp::v4(), endpoints, ec);

    assert(ec);
    assert(endpoints.empty());
    std::cout << "  expected error: " << ec.message() << "\n";
}

static void test_resolve_udp()
{
    std::cout << "[test_resolve_udp]\n";

    ip::udp::resolver resolver;
    std::list<ip::udp::endpoint> endpoints;

    resolver.resolve("8.8.8.8", "domain", ip::udp::v4(), endpoints);

    assert(!endpoints.empty());
    assert(endpoints.front().to_string() == "8.8.8.8:53");
    std::cout << "  " << endpoints.front().to_string() << "\n";
}

static void test_resolve_throw_ok()
{
    std::cout << "[test_resolve_throw_ok]\n";

    ip::tcp::resolver resolver;
    std::list<ip::tcp::endpoint> endpoints;

    try {
        resolver.resolve("127.0.0.1", "80", ip::tcp::v4(), endpoints);
        assert(!endpoints.empty());
        std::cout << "  no throw, ok\n";
    } catch ( const std::exception& e ) {
        std::cerr << "  unexpected throw: " << e.what() << "\n";
        assert(false);
    }
}

static void test_resolve_throw_fail()
{
    std::cout << "[test_resolve_throw_fail]\n";

    ip::tcp::resolver resolver;
    std::list<ip::tcp::endpoint> endpoints;

    bool thrown = false;
    try {
        resolver.resolve("this.domain.should.not.exist.invalid",
            "http", ip::tcp::v4(), endpoints);
    } catch ( const std::exception& e ) {
        thrown = true;
        std::cout << "  expected throw: " << e.what() << "\n";
    }
    assert(thrown);
}

static void test_resolve_localhost()
{
    std::cout << "[test_resolve_localhost]\n";

    ip::tcp::resolver resolver;
    std::list<ip::tcp::endpoint> endpoints;

    lcy::system::error_code ec;
    resolver.resolve("localhost", "80", ip::tcp::v4(), endpoints, ec);

    if ( ec ) {
        std::cout << "  localhost resolve failed (no /etc/hosts?): "
                  << ec.message() << "\n";
        return;
    }

    assert(!endpoints.empty());
    for ( auto& ep : endpoints ) {
        assert(ep.is_v4());
        std::cout << "  " << ep.to_string() << "\n";
    }
}

int main()
{
    test_resolve_host_service();
    test_resolve_ip_port();
    test_resolve_fail();
    test_resolve_udp();
    test_resolve_throw_ok();
    test_resolve_throw_fail();
    test_resolve_localhost();

    std::cout << "\nall resolver tests passed\n";
    return 0;
}