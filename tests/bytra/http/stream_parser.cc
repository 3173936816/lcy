#include <cassert>
#include <cstring>
#include <iostream>
#include <string>

#include "lcy/bytra/http/detail/stream_parser.hpp"

namespace {

int g_passed = 0;
int g_failed = 0;

#define RUN_TEST(fn)                                            \
    do {                                                        \
        std::cout << "[ RUN      ] " << #fn << std::endl;       \
        fn();                                                   \
        std::cout << "[       OK ] " << #fn << std::endl;       \
        ++g_passed;                                             \
    } while ( 0 )

using parser_t = lcy::bytra::http::detail::stream_parser;
using phase_t  = parser_t::parse_phase;
using result_t = parser_t::parse_result;

std::string to_string(const parser_t::data_view& v)
{
    if ( v.ptr_ == nullptr || v.length_ == 0 ) { return std::string(); }
    return std::string(v.ptr_, v.length_);
}

void advance_to_field_line(parser_t& p)
{
    p.parse_first_line("GET / HTTP/1.1\r\n", 16,
        [](lcy::system::error_code, const parser_t::first_line_view&, result_t) {});
}

void advance_to_body(parser_t& p)
{
    advance_to_field_line(p);
    p.parse_field_line("\r\n", 2,
        [](lcy::system::error_code, const parser_t::field_line_view&, result_t) {});
}

bool is_http_error(const lcy::system::error_code& ec,
                   lcy::bytra::http::error expected)
{
    return ec == static_cast<int>(expected);
}

void test_first_line_request()
{
    std::cout << "  case: GET /index.html HTTP/1.1" << std::endl;
    parser_t p;
    const std::string line = "GET /index.html HTTP/1.1\r\n";
    std::string a, b, c;
    result_t r{};
    std::size_t n = p.parse_first_line(line.data(), line.size(),
        [&](lcy::system::error_code ec, const parser_t::first_line_view& v, result_t res) {
            assert(!ec);
            r = res;
            a = to_string(v.first_);
            b = to_string(v.second_);
            c = to_string(v.third_);
        });
    assert(r == result_t::success);
    assert(n == line.size());
    assert(a == "GET");
    assert(b == "/index.html");
    assert(c == "HTTP/1.1");
    assert(p.get_parse_phase() == phase_t::field_line);
}

void test_first_line_response()
{
    std::cout << "  case: HTTP/1.1 200 OK" << std::endl;
    parser_t p;
    const std::string line = "HTTP/1.1 200 OK\r\n";
    std::string a, b, c;
    result_t r{};
    p.parse_first_line(line.data(), line.size(),
        [&](lcy::system::error_code, const parser_t::first_line_view& v, result_t res) {
            r = res;
            a = to_string(v.first_);
            b = to_string(v.second_);
            c = to_string(v.third_);
        });
    assert(r == result_t::success);
    assert(a == "HTTP/1.1");
    assert(b == "200");
    assert(c == "OK");
}

void test_first_line_wait_more()
{
    std::cout << "  case: half line -> wait_more" << std::endl;
    parser_t p;
    result_t r{};
    std::size_t n = p.parse_first_line("GET /index", 10,
        [&](lcy::system::error_code, const parser_t::first_line_view&, result_t res) {
            r = res;
        });
    assert(r == result_t::wait_more);
    assert(n == 0);
    assert(p.get_parse_phase() == phase_t::first_line);
}

void test_first_line_bad_format()
{
    std::cout << "  case: only 2 tokens -> bad_protocol" << std::endl;
    parser_t p;
    result_t r{};
    p.parse_first_line("GET /\r\n", 7,
        [&](lcy::system::error_code, const parser_t::first_line_view&, result_t res) {
            r = res;
        });
    assert(r == result_t::error_occurred);
}

void test_field_line_normal()
{
    std::cout << "  case: Host: example.com" << std::endl;
    parser_t p;
    advance_to_field_line(p);
    std::string k, v;
    result_t r{};
    const std::string line = "Host: example.com\r\n";
    std::size_t n = p.parse_field_line(line.data(), line.size(),
        [&](lcy::system::error_code, const parser_t::field_line_view& fv, result_t res) {
            r = res;
            k = to_string(fv.key_);
            v = to_string(fv.value_);
        });
    assert(r == result_t::success);
    assert(n == line.size());
    assert(k == "Host");
    assert(v == "example.com");
}

void test_field_line_trim()
{
    std::cout << "  case: X-Test:   value  " << std::endl;
    parser_t p;
    advance_to_field_line(p);
    std::string k, v;
    p.parse_field_line("X-Test:   value  \r\n", 19,
        [&](lcy::system::error_code, const parser_t::field_line_view& fv, result_t) {
            k = to_string(fv.key_);
            v = to_string(fv.value_);
        });
    assert(k == "X-Test");
    assert(v == "value  ");
}

void test_field_line_empty_ends_header()
{
    std::cout << "  case: empty line -> phase body" << std::endl;
    parser_t p;
    advance_to_field_line(p);
    result_t r{};
    std::size_t n = p.parse_field_line("\r\n", 2,
        [&](lcy::system::error_code, const parser_t::field_line_view&, result_t res) {
            r = res;
        });
    assert(r == result_t::success);
    assert(n == 2);
    assert(p.get_parse_phase() == phase_t::body);
}

void test_field_line_missing_colon()
{
    std::cout << "  case: no colon -> bad_protocol" << std::endl;
    parser_t p;
    advance_to_field_line(p);
    result_t r{};
    p.parse_field_line("NoColon\r\n", 9,
        [&](lcy::system::error_code ec, const parser_t::field_line_view&, result_t res) {
            assert(ec);
            r = res;
        });
    assert(r == result_t::error_occurred);
}

void test_body_known_length_complete()
{
    std::cout << "  case: body_remaining 5, push 5 -> complete" << std::endl;
    parser_t p;
    advance_to_body(p);
    p.set_body_remaining(5);

    std::string got;
    result_t r{};
    std::size_t n = p.parse_body("hello", 5,
        [&](lcy::system::error_code ec, const parser_t::body_view& v, result_t res) {
            assert(!ec);
            r = res;
            got = to_string(v.body_);
        });
    assert(r == result_t::success);
    assert(n == 5);
    assert(got == "hello");
    assert(p.get_parse_phase() == phase_t::complete);
}

void test_body_partial_keeps_body_phase()
{
    std::cout << "  case: body_remaining 10, push 5 -> still body" << std::endl;
    parser_t p;
    advance_to_body(p);
    p.set_body_remaining(10);
    std::size_t n = p.parse_body("hello", 5,
        [](lcy::system::error_code, const parser_t::body_view&, result_t) {});
    assert(n == 5);
    assert(p.get_parse_phase() == phase_t::body);
}

void test_body_total_limit()
{
    std::cout << "  case: body_total_limit exceeded" << std::endl;
    parser_t p;
    advance_to_body(p);
    p.set_body_remaining(100);
    p.set_body_total_limit(10);

    result_t r{};
    std::size_t n = p.parse_body("0123456789ABCDEF", 16,
        [&](lcy::system::error_code ec, const parser_t::body_view&, result_t res) {
            assert(ec);
            r = res;
        });
    assert(r == result_t::error_occurred);
    assert(n == 0);
}

void test_chunked_complete()
{
    std::cout << "  case: 5\r\nhello\r\n0\r\n\r\n" << std::endl;
    parser_t p;
    advance_to_body(p);
    p.set_chunk_mode(true);

    const std::string data = "5\r\nhello\r\n0\r\n\r\n";
    std::string payload;
    std::size_t off = 0;
    while ( off < data.size() && p.get_parse_phase() == phase_t::body ) {
        std::size_t n = p.parse_chunk(data.data() + off, data.size() - off,
            [&](lcy::system::error_code ec, const parser_t::chunk_view& v, result_t r) {
                assert(!ec);
                assert(r == result_t::success);
                if ( v.payload_.length_ > 0 ) {
                    payload.append(v.payload_.ptr_, v.payload_.length_);
                }
            });
        if ( n == 0 ) { break; }
        off += n;
    }
    assert(payload == "hello");
    assert(p.get_parse_phase() == phase_t::complete);
}

void test_chunked_wait_more()
{
    std::cout << "  case: 5\r\nhel -> wait_more" << std::endl;
    parser_t p;
    advance_to_body(p);
    p.set_chunk_mode(true);

    std::size_t n = p.parse_chunk("5\r\nhel", 6,
        [](lcy::system::error_code, const parser_t::chunk_view&, result_t) {});
    assert(n == 6);
    assert(p.get_parse_phase() == phase_t::body);

    bool got_wait_more = false;
    n = p.parse_chunk("", 0,
        [&](lcy::system::error_code, const parser_t::chunk_view&, result_t r) {
            if ( r == result_t::wait_more ) { got_wait_more = true; }
        });
    assert(got_wait_more);
    assert(n == 0);
    assert(p.get_parse_phase() == phase_t::body);
}

void test_chunked_bad_size()
{
    std::cout << "  case: ZZZ -> bad_protocol" << std::endl;
    parser_t p;
    advance_to_body(p);
    p.set_chunk_mode(true);

    result_t r{};
    p.parse_chunk("ZZZ\r\n", 5,
        [&](lcy::system::error_code ec, const parser_t::chunk_view&, result_t res) {
            assert(ec);
            r = res;
        });
    assert(r == result_t::error_occurred);
}

void test_chunked_payload_limit()
{
    std::cout << "  case: chunk size > payload_limit" << std::endl;
    parser_t p;
    advance_to_body(p);
    p.set_chunk_mode(true);
    p.set_chunk_payload_limit(4);

    result_t r{};
    p.parse_chunk("10\r\n", 4,
        [&](lcy::system::error_code ec, const parser_t::chunk_view&, result_t res) {
            assert(ec);
            r = res;
        });
    assert(r == result_t::error_occurred);
}

void test_trailer_complete()
{
    std::cout << "  case: chunked + trailer -> complete" << std::endl;
    parser_t p;
    advance_to_body(p);
    p.set_chunk_mode(true);
    p.set_trailer_check(true);

    std::size_t n = p.parse_chunk("0\r\n\r\n", 5,
        [](lcy::system::error_code ec, const parser_t::chunk_view&, result_t r) {
            assert(!ec);
            assert(r == result_t::success);
        });
    assert(n == 5);
    assert(p.get_parse_phase() == phase_t::trailer);

    std::string k, v;
    std::size_t off = 0;
    const std::string trailer = "X-Trailer: abc\r\n\r\n";
    while ( off < trailer.size() && p.get_parse_phase() != phase_t::complete ) {
        std::size_t m = p.parse_trailer(trailer.data() + off, trailer.size() - off,
            [&](lcy::system::error_code ec, const parser_t::field_line_view& fv, result_t r) {
                assert(!ec);
                if ( r == result_t::success && fv.key_.length_ > 0 ) {
                    k = to_string(fv.key_);
                    v = to_string(fv.value_);
                }
            });
        if ( m == 0 ) { break; }
        off += m;
    }
    assert(p.get_parse_phase() == phase_t::complete);
    assert(k == "X-Trailer");
    assert(v == "abc");
}

void test_phase_mismatch_body()
{
    std::cout << "  case: parse_body at first_line phase" << std::endl;
    parser_t p;
    result_t r{};
    std::size_t n = p.parse_body("x", 1,
        [&](lcy::system::error_code ec, const parser_t::body_view&, result_t res) {
            assert(ec);
            r = res;
        });
    assert(n == 0);
    assert(r == result_t::error_occurred);
}

void test_phase_mismatch_chunk()
{
    std::cout << "  case: parse_chunk at body but not chunk mode" << std::endl;
    parser_t p;
    advance_to_body(p);
    result_t r{};
    p.parse_chunk("0\r\n\r\n", 5,
        [&](lcy::system::error_code ec, const parser_t::chunk_view&, result_t res) {
            assert(ec);
            r = res;
        });
    assert(r == result_t::error_occurred);
}

void test_phase_mismatch_trailer()
{
    std::cout << "  case: parse_trailer at body phase" << std::endl;
    parser_t p;
    advance_to_body(p);
    result_t r{};
    p.parse_trailer("X: y\r\n", 6,
        [&](lcy::system::error_code ec, const parser_t::field_line_view&, result_t res) {
            assert(ec);
            r = res;
        });
    assert(r == result_t::error_occurred);
}

void test_error_code_phase_mismatch()
{
    std::cout << "  case: phase_mismatch code" << std::endl;
    parser_t p;
    lcy::system::error_code captured;
    p.parse_body("x", 1,
        [&](lcy::system::error_code ec, const parser_t::body_view&, result_t) {
            captured = ec;
        });
    assert(is_http_error(captured, lcy::bytra::http::error::phase_mismatch));
}

void test_error_code_bad_protocol()
{
    std::cout << "  case: bad_protocol code" << std::endl;
    parser_t p;
    lcy::system::error_code captured;
    p.parse_first_line("GET /\r\n", 7,
        [&](lcy::system::error_code ec, const parser_t::first_line_view&, result_t) {
            captured = ec;
        });
    assert(is_http_error(captured, lcy::bytra::http::error::bad_protocol));
}

void test_error_code_first_line_limit()
{
    std::cout << "  case: first_line_limit code" << std::endl;
    parser_t p;
    p.set_first_line_limit(10);
    lcy::system::error_code captured;
    p.parse_first_line("GET /very/long/path HTTP/1.1\r\n", 30,
        [&](lcy::system::error_code ec, const parser_t::first_line_view&, result_t) {
            captured = ec;
        });
    assert(is_http_error(captured, lcy::bytra::http::error::first_line_limit));
}

void test_error_code_field_total_limit()
{
    std::cout << "  case: field_total_limit code" << std::endl;
    parser_t p;
    p.set_field_total_limit(10);
    advance_to_field_line(p);
    lcy::system::error_code captured;
    p.parse_field_line("Host: example.com\r\n", 19,
        [&](lcy::system::error_code ec, const parser_t::field_line_view&, result_t) {
            captured = ec;
        });
    assert(is_http_error(captured, lcy::bytra::http::error::field_total_limit));
}

void test_error_code_chunk_payload_limit()
{
    std::cout << "  case: chunk_payload_limit code" << std::endl;
    parser_t p;
    advance_to_body(p);
    p.set_chunk_mode(true);
    p.set_chunk_payload_limit(4);
    lcy::system::error_code captured;
    p.parse_chunk("10\r\n", 4,
        [&](lcy::system::error_code ec, const parser_t::chunk_view&, result_t) {
            captured = ec;
        });
    assert(is_http_error(captured, lcy::bytra::http::error::chunk_payload_limit));
}

void test_error_code_body_total_limit()
{
    std::cout << "  case: body_total_limit code" << std::endl;
    parser_t p;
    advance_to_body(p);
    p.set_body_total_limit(3);
    p.set_body_remaining(10);
    lcy::system::error_code captured;
    p.parse_body("hello", 5,
        [&](lcy::system::error_code ec, const parser_t::body_view&, result_t) {
            captured = ec;
        });
    assert(is_http_error(captured, lcy::bytra::http::error::body_total_limit));
}

void test_reset()
{
    std::cout << "  case: reset -> first_line, bytes 0" << std::endl;
    parser_t p;
    p.set_first_line_limit(100);
    advance_to_field_line(p);
    assert(p.get_parse_bytes() > 0);

    p.reset();
    assert(p.get_parse_phase() == phase_t::first_line);
    assert(p.get_parse_bytes() == 0);

    std::string long_line(200, 'a');
    long_line += "\r\n";
    lcy::system::error_code captured;
    p.parse_first_line(long_line.data(), long_line.size(),
        [&](lcy::system::error_code ec, const parser_t::first_line_view&, result_t) {
            captured = ec;
        });
    assert(is_http_error(captured, lcy::bytra::http::error::first_line_limit));
}

} // namespace

int main()
{
    std::cout << "===== stream_parser tests =====" << std::endl;

    RUN_TEST(test_first_line_request);
    RUN_TEST(test_first_line_response);
    RUN_TEST(test_first_line_wait_more);
    RUN_TEST(test_first_line_bad_format);
    RUN_TEST(test_field_line_normal);
    RUN_TEST(test_field_line_trim);
    RUN_TEST(test_field_line_empty_ends_header);
    RUN_TEST(test_field_line_missing_colon);
    RUN_TEST(test_body_known_length_complete);
    RUN_TEST(test_body_partial_keeps_body_phase);
    RUN_TEST(test_body_total_limit);
    RUN_TEST(test_chunked_complete);
    RUN_TEST(test_chunked_wait_more);
    RUN_TEST(test_chunked_bad_size);
    RUN_TEST(test_chunked_payload_limit);
    RUN_TEST(test_trailer_complete);
    RUN_TEST(test_phase_mismatch_body);
    RUN_TEST(test_phase_mismatch_chunk);
    RUN_TEST(test_phase_mismatch_trailer);
    RUN_TEST(test_error_code_phase_mismatch);
    RUN_TEST(test_error_code_bad_protocol);
    RUN_TEST(test_error_code_first_line_limit);
    RUN_TEST(test_error_code_field_total_limit);
    RUN_TEST(test_error_code_chunk_payload_limit);
    RUN_TEST(test_error_code_body_total_limit);
    RUN_TEST(test_reset);

    std::cout << "===== " << g_passed << " passed, "
              << g_failed << " failed =====" << std::endl;
    return g_failed == 0 ? 0 : 1;
}