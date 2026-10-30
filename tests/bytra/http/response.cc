// test_response.cc
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "lcy/bytra/flat_buffer.hpp"
#include "lcy/bytra/http/response.hpp"
#include "lcy/bytra/http/status.hpp"
#include "lcy/bytra/http/version.hpp"

using namespace lcy::bytra::http;

static void expect_str_eq(const std::string& got, const std::string& want,
                          const char* msg)
{
    if ( got != want ) {
        std::cerr << "FAIL: " << msg << "\n"
                  << "  got : [" << got << "]\n"
                  << "  want: [" << want << "]\n";
        std::abort();
    }
}

static void expect_contains(const std::string& got, const std::string& sub,
                            const char* msg)
{
    if ( got.find(sub) == std::string::npos ) {
        std::cerr << "FAIL: " << msg << "\n"
                  << "  got : [" << got << "]\n"
                  << "  want contains: [" << sub << "]\n";
        std::abort();
    }
}

static void expect_starts_with(const std::string& got, const std::string& prefix,
                               const char* msg)
{
    if ( got.compare(0, prefix.size(), prefix) != 0 ) {
        std::cerr << "FAIL: " << msg << "\n"
                  << "  got : [" << got << "]\n"
                  << "  want prefix: [" << prefix << "]\n";
        std::abort();
    }
}

static void expect_size(std::size_t got, std::size_t want, const char* msg)
{
    if ( got != want ) {
        std::cerr << "FAIL: " << msg << "\n  got : " << got
                  << "\n  want: " << want << "\n";
        std::abort();
    }
}

static void expect_true(bool cond, const char* msg)
{
    if ( !cond ) {
        std::cerr << "FAIL: " << msg << "\n";
        std::abort();
    }
}

static void test_default_construct()
{
    std::cout << "[test_default_construct]\n";
    response<lcy::bytra::flat_buffer> res;

    expect_true(res.get_status() == status::ok, "default status ok");

    std::string s = res.header_to_string();
    expect_starts_with(s, "200 ", "default status line starts with 200");
    expect_contains(s, "OK", "default reason OK");
    expect_contains(s, "HTTP/1.", "version present");
    expect_contains(s, "\r\n\r\n", "empty header terminator");

    std::cout << "  default construct ok\n";
}

static void test_construct_status_version()
{
    std::cout << "[test_construct_status_version]\n";
    response<lcy::bytra::flat_buffer> res(status::not_found, version::http_1_1);

    expect_true(res.get_status() == status::not_found, "status not_found");
    expect_str_eq(res.get_reason(), "Not Found", "reason Not Found");

    std::string s = res.header_to_string();
    expect_starts_with(s, "404 Not Found HTTP/1.1\r\n", "status line");

    std::cout << "  construct status/version ok\n";
}

static void test_construct_custom_reason()
{
    std::cout << "[test_construct_custom_reason]\n";
    response<lcy::bytra::flat_buffer> res(
        status::ok, "Everything Fine", version::http_1_1);

    expect_true(res.get_status() == status::ok, "status ok");
    expect_str_eq(res.get_reason(), "Everything Fine", "custom reason");

    std::string s = res.header_to_string();
    expect_starts_with(s, "200 Everything Fine HTTP/1.1\r\n", "status line");

    std::cout << "  construct custom reason ok\n";
}

static void test_set_status_resets_reason()
{
    std::cout << "[test_set_status_resets_reason]\n";
    response<lcy::bytra::flat_buffer> res;
    res.set_reason("Custom");
    expect_str_eq(res.get_reason(), "Custom", "reason before set_status");

    res.set_status(status::internal_server_error);
    expect_true(res.get_status() == status::internal_server_error, "status 500");
    expect_str_eq(res.get_reason(), "Internal Server Error",
                  "reason reset by set_status");

    std::string s = res.header_to_string();
    expect_starts_with(s, "500 Internal Server Error ",
                       "status line after set_status");

    std::cout << "  set_status resets reason ok\n";
}

static void test_set_reason()
{
    std::cout << "[test_set_reason]\n";
    response<lcy::bytra::flat_buffer> res(status::ok, version::http_1_1);

    res.set_reason("All Good");
    expect_str_eq(res.get_reason(), "All Good", "reason override");
    expect_true(res.get_status() == status::ok, "status unchanged");

    std::string s = res.header_to_string();
    expect_starts_with(s, "200 All Good HTTP/1.1\r\n", "status line");

    std::cout << "  set_reason ok\n";
}

static void test_single_header()
{
    std::cout << "[test_single_header]\n";
    response<lcy::bytra::flat_buffer> res(status::ok, version::http_1_1);
    res.set_field("Content-Type", "text/plain");

    std::string s = res.header_to_string();
    expect_contains(s, "Content-Type: text/plain\r\n", "Content-Type header");

    std::cout << "  single header ok\n";
}

static void test_multi_headers_order()
{
    std::cout << "[test_multi_headers_order]\n";
    response<lcy::bytra::flat_buffer> res(status::ok, version::http_1_1);
    res.set_field("A", "1");
    res.set_field("B", "2");
    res.set_field("C", "3");

    std::string s = res.header_to_string();
    auto pa = s.find("A: 1");
    auto pb = s.find("B: 2");
    auto pc = s.find("C: 3");
    if ( pa == std::string::npos || pb == std::string::npos
         || pc == std::string::npos || !(pa < pb && pb < pc) ) {
        std::cerr << "FAIL: multi headers order\n  got: [" << s << "]\n";
        std::abort();
    }

    std::cout << "  multi headers in order ok\n";
}

static void test_duplicate_header()
{
    std::cout << "[test_duplicate_header]\n";
    response<lcy::bytra::flat_buffer> res(status::ok, version::http_1_1);
    res.set_field("Set-Cookie", "a=1");
    res.set_field("Set-Cookie", "b=2");

    std::string s = res.header_to_string();
    expect_contains(s, "Set-Cookie: a=1\r\n", "first cookie");
    expect_contains(s, "Set-Cookie: b=2\r\n", "second cookie");
    expect_size(res.count("Set-Cookie"), 2, "cookie count 2");

    std::cout << "  duplicate header ok\n";
}

static void test_field_accessors()
{
    std::cout << "[test_field_accessors]\n";
    response<lcy::bytra::flat_buffer> res(status::ok, version::http_1_1);
    res.set_field("X-A", "va");
    res.set_field("X-B", "vb");
    res.set_field("X-A", "va2");

    expect_size(res.count("X-A"), 2, "count X-A");
    expect_size(res.count("X-B"), 1, "count X-B");
    expect_size(res.count("X-C"), 0, "count X-C");

    auto it = res.get_field("X-A");
    expect_true(it != res.field_end(), "get_field X-A found");
    expect_true(it->second == "va", "get_field first value va");

    expect_true(res.get_field("X-C") == res.field_end(), "missing is end");

    res.remove_fields("X-A");
    expect_size(res.count("X-A"), 0, "count X-A after remove");
    expect_size(res.count("X-B"), 1, "count X-B after remove");

    std::cout << "  field accessors ok\n";
}

static void test_common_statuses()
{
    std::cout << "[test_common_statuses]\n";
    response<lcy::bytra::flat_buffer> res;

    struct { status s; const char* reason_prefix; } cases[] = {
        { status::ok,                     "200" },
        { status::created,                "201" },
        { status::no_content,             "204" },
        { status::moved_permanently,      "301" },
        { status::found,                  "302" },
        { status::not_modified,           "304" },
        { status::bad_request,            "400" },
        { status::unauthorized,           "401" },
        { status::forbidden,              "403" },
        { status::not_found,              "404" },
        { status::method_not_allowed,     "405" },
        { status::internal_server_error,  "500" },
        { status::not_implemented,        "501" },
        { status::bad_gateway,            "502" },
        { status::service_unavailable,    "503" },
    };

    for ( auto& c : cases ) {
        res.set_status(c.s);
        std::string s = res.header_to_string();
        expect_starts_with(s, std::string(c.reason_prefix) + " ",
                           c.reason_prefix);
        expect_true(!res.get_reason().empty(), "reason not empty");
    }

    std::cout << "  common statuses ok\n";
}

static void test_versions()
{
    std::cout << "[test_versions]\n";
    response<lcy::bytra::flat_buffer> res;

    res.set_version(version::http_1_0);
    expect_contains(res.header_to_string(), "HTTP/1.0", "http_1_0");

    res.set_version(version::http_1_1);
    expect_contains(res.header_to_string(), "HTTP/1.1", "http_1_1");

    std::cout << "  versions ok\n";
}

static void test_body()
{
    std::cout << "[test_body]\n";
    response<lcy::bytra::flat_buffer> res(status::ok, version::http_1_1);
    res.set_field("Content-Length", "5");

    auto buf = res.body_container().prepare(5);
    expect_true(buf.length() >= 5, "prepare length >= 5");
    std::memcpy(buf.data(), "hello", 5);
    res.body_container().commit(5);

    expect_size(res.body_container().size(), 5, "body size 5");

    std::string h = res.header_to_string();
    expect_true(h.find("hello") == std::string::npos,
                "header_to_string should not contain body");
    expect_contains(h, "Content-Length: 5", "content-length");

    std::cout << "  body ok\n";
}

static void test_format_strict()
{
    std::cout << "[test_format_strict]\n";
    response<lcy::bytra::flat_buffer> res(status::ok, version::http_1_1);
    res.set_field("A", "1");
    res.set_field("B", "2");

    std::string got = res.header_to_string();
    std::string want =
        "200 OK HTTP/1.1\r\n"
        "A: 1\r\n"
        "B: 2\r\n"
        "\r\n";
    expect_str_eq(got, want, "strict header_to_string");

    std::cout << "  strict format ok\n";
}

int main()
{
    test_default_construct();
    test_construct_status_version();
    test_construct_custom_reason();
    test_set_status_resets_reason();
    test_set_reason();
    test_single_header();
    test_multi_headers_order();
    test_duplicate_header();
    test_field_accessors();
    test_common_statuses();
    test_versions();
    test_body();
    test_format_strict();

    std::cout << "\nall response tests passed\n";
    return 0;
}