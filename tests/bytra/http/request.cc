// test_request.cc
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>

#include "lcy/bytra/flat_buffer.hpp"
#include "lcy/bytra/http/request.hpp"
#include "lcy/bytra/http/method.hpp"
#include "lcy/bytra/http/version.hpp"

using namespace lcy::bytra::http;

static void expect_contains(const std::string& s, const std::string& sub,
                            const char* msg)
{
    if ( s.find(sub) == std::string::npos ) {
        std::cerr << "FAIL: " << msg << "\n  got: [" << s << "]\n"
                  << "  want contains: [" << sub << "]\n";
        std::abort();
    }
}

static void expect_starts_with(const std::string& s, const std::string& prefix,
                               const char* msg)
{
    if ( s.compare(0, prefix.size(), prefix) != 0 ) {
        std::cerr << "FAIL: " << msg << "\n  got: [" << s << "]\n"
                  << "  want prefix: [" << prefix << "]\n";
        std::abort();
    }
}

static void test_default_construct()
{
    std::cout << "[test_default_construct]\n";
    request<lcy::bytra::flat_buffer> req;

    assert(req.get_method() == method::get);
    assert(req.get_target() == "/");

    std::string s = req.header_to_string();
    expect_starts_with(s, "GET / ", "default request line");
    expect_contains(s, "HTTP/1.", "version present");
    assert(s.find("\r\n\r\n") != std::string::npos);

    std::cout << "  default ok\n";
}

static void test_construct_with_args()
{
    std::cout << "[test_construct_with_args]\n";
    request<lcy::bytra::flat_buffer> req(
        method::post, "/api/v1/users", version::http_1_1);

    assert(req.get_method() == method::post);
    assert(req.get_target() == "/api/v1/users");
    assert(req.get_version() == version::http_1_1);

    std::string s = req.header_to_string();
    expect_starts_with(s, "POST /api/v1/users HTTP/1.1\r\n", "request line");

    std::cout << "  construct with args ok\n";
}

static void test_setters()
{
    std::cout << "[test_setters]\n";
    request<lcy::bytra::flat_buffer> req;

    req.set_method(method::put);
    req.set_target("/update");
    req.set_version(version::http_1_0);

    assert(req.get_method() == method::put);
    assert(req.get_target() == "/update");
    assert(req.get_version() == version::http_1_0);

    std::string s = req.header_to_string();
    expect_starts_with(s, "PUT /update HTTP/1.0\r\n", "after setters");

    std::cout << "  setters ok\n";
}

static void test_single_header()
{
    std::cout << "[test_single_header]\n";
    request<lcy::bytra::flat_buffer> req;
    req.set_field("Host", "example.com");

    std::string s = req.header_to_string();
    expect_contains(s, "Host: example.com\r\n", "Host header");
    assert(s.find("\r\n\r\n") != std::string::npos);

    std::cout << "  single header ok\n";
}

static void test_multi_headers()
{
    std::cout << "[test_multi_headers]\n";
    request<lcy::bytra::flat_buffer> req;
    req.set_field("A", "1");
    req.set_field("B", "2");
    req.set_field("C", "3");

    std::string s = req.header_to_string();
    auto pa = s.find("A: 1");
    auto pb = s.find("B: 2");
    auto pc = s.find("C: 3");
    assert(pa != std::string::npos);
    assert(pb != std::string::npos);
    assert(pc != std::string::npos);
    assert(pa < pb && pb < pc);

    std::cout << "  multi headers in order ok\n";
}

static void test_duplicate_header()
{
    std::cout << "[test_duplicate_header]\n";
    request<lcy::bytra::flat_buffer> req;
    req.set_field("Set-Cookie", "a=1");
    req.set_field("Set-Cookie", "b=2");

    std::string s = req.header_to_string();
    expect_contains(s, "Set-Cookie: a=1\r\n", "first cookie");
    expect_contains(s, "Set-Cookie: b=2\r\n", "second cookie");
    assert(req.count("Set-Cookie") == 2);

    std::cout << "  duplicate header ok\n";
}

static void test_field_accessors()
{
    std::cout << "[test_field_accessors]\n";
    request<lcy::bytra::flat_buffer> req;
    req.set_field("X-A", "va");
    req.set_field("X-B", "vb");
    req.set_field("X-A", "va2");

    assert(req.count("X-A") == 2);
    assert(req.count("X-B") == 1);
    assert(req.count("X-C") == 0);

    auto it = req.get_field("X-A");
    assert(it != req.field_end());
    assert(it->first == "X-A");
    assert(it->second == "va");

    auto miss = req.get_field("X-C");
    assert(miss == req.field_end());

    req.remove_fields("X-A");
    assert(req.count("X-A") == 0);
    assert(req.count("X-B") == 1);

    std::cout << "  field accessors ok\n";
}

static void test_special_target()
{
    std::cout << "[test_special_target]\n";
    request<lcy::bytra::flat_buffer> req;
    req.set_target("/a/b?x=1&y=2#frag");

    std::string s = req.header_to_string();
    expect_contains(s, "/a/b?x=1&y=2#frag", "target with query");

    req.set_target("");
    s = req.header_to_string();
    expect_starts_with(s, "GET  ", "empty target still has space");

    std::cout << "  special target ok\n";
}

static void test_all_methods()
{
    std::cout << "[test_all_methods]\n";
    request<lcy::bytra::flat_buffer> req;

    struct { method m; const char* name; } cases[] = {
        { method::get,     "GET" },
        { method::post,    "POST" },
        { method::put,     "PUT" },
        { method::delete_, "DELETE" },
        { method::head,    "HEAD" },
        { method::options, "OPTIONS" },
        { method::patch,   "PATCH" },
    };

    for ( auto& c : cases ) {
        req.set_method(c.m);
        std::string s = req.header_to_string();
        expect_starts_with(s, std::string(c.name) + " ", c.name);
    }
    std::cout << "  all methods ok\n";
}

static void test_versions()
{
    std::cout << "[test_versions]\n";
    request<lcy::bytra::flat_buffer> req;

    req.set_version(version::http_1_0);
    expect_contains(req.header_to_string(), "HTTP/1.0", "http_1_0");

    req.set_version(version::http_1_1);
    expect_contains(req.header_to_string(), "HTTP/1.1", "http_1_1");

    std::cout << "  versions ok\n";
}

static void test_body()
{
    std::cout << "[test_body]\n";
    request<lcy::bytra::flat_buffer> req;
    req.set_field("Content-Length", "5");

    auto buf = req.body_container().prepare(5);
    assert(buf.length() >= 5);
    std::memcpy(buf.data(), "hello", 5);
    req.body_container().commit(5);

    assert(req.body_container().size() == 5);
    assert(std::string(
        static_cast<const char*>(req.body_container().data()), 5) == "hello");

    std::string h = req.header_to_string();
    assert(h.find("hello") == std::string::npos);
    expect_contains(h, "Content-Length: 5", "content-length");

    std::cout << "  body ok\n";
}

static void test_format_strict()
{
    std::cout << "[test_format_strict]\n";
    request<lcy::bytra::flat_buffer> req(
        method::get, "/", version::http_1_1);
    req.set_field("A", "1");
    req.set_field("B", "2");

    std::string s = req.header_to_string();
    std::string expected =
        "GET / HTTP/1.1\r\n"
        "A: 1\r\n"
        "B: 2\r\n"
        "\r\n";
    assert(s == expected);

    std::cout << "  strict format ok\n";
}

int main()
{
    test_default_construct();
    test_construct_with_args();
    test_setters();
    test_single_header();
    test_multi_headers();
    test_duplicate_header();
    test_field_accessors();
    test_special_target();
    test_all_methods();
    test_versions();
    test_body();
    test_format_strict();

    std::cout << "\nall request tests passed\n";
    return 0;
}