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

struct first_line_outcome
{
    std::size_t consumed = 0;
    phase_t     phase = phase_t::first_line;
    bool        success = false;
    bool        wait_more = false;
    bool        error = false;
    std::string first, second, third;
    lcy::system::error_code ec;
};

first_line_outcome do_first_line(parser_t& p, const std::string& input)
{
    first_line_outcome out;
    out.consumed = p.parse_first_line(input.data(), input.size(),
        [&](lcy::system::error_code ec,
            const parser_t::first_line_view& v,
            result_t r) {
            out.ec = ec;
            out.success   = (r == result_t::success);
            out.wait_more = (r == result_t::wait_more);
            out.error     = (r == result_t::error_occurred);
            if ( r == result_t::success ) {
                out.first  = to_string(v.first_);
                out.second = to_string(v.second_);
                out.third  = to_string(v.third_);
            }
        });
    out.phase = p.get_parse_phase();
    return out;
}

struct field_outcome
{
    std::size_t consumed = 0;
    phase_t     phase = phase_t::first_line;
    bool        success = false;
    bool        wait_more = false;
    bool        error = false;
    std::string key, value;
    lcy::system::error_code ec;
};

field_outcome do_field_line(parser_t& p, const std::string& input)
{
    field_outcome out;
    out.consumed = p.parse_field_line(input.data(), input.size(),
        [&](lcy::system::error_code ec,
            const parser_t::field_line_view& v,
            result_t r) {
            out.ec = ec;
            out.success   = (r == result_t::success);
            out.wait_more = (r == result_t::wait_more);
            out.error     = (r == result_t::error_occurred);
            if ( r == result_t::success ) {
                out.key   = to_string(v.key_);
                out.value = to_string(v.value_);
            }
        });
    out.phase = p.get_parse_phase();
    return out;
}

void advance_to_body(parser_t& p)
{
    do_first_line(p, "GET / HTTP/1.1\r\n");
    do_field_line(p, "\r\n");
}

void test_first_line_request()
{
    std::cout << "  case: GET /index.html HTTP/1.1" << std::endl;
    parser_t p;
    auto out = do_first_line(p, "GET /index.html HTTP/1.1\r\n");
    assert(out.success);
    assert(out.consumed == std::string("GET /index.html HTTP/1.1\r\n").size());
    assert(out.first  == "GET");
    assert(out.second == "/index.html");
    assert(out.third  == "HTTP/1.1");
    assert(out.phase  == phase_t::field_line);
}

void test_first_line_response()
{
    std::cout << "  case: HTTP/1.1 200 OK" << std::endl;
    parser_t p;
    auto out = do_first_line(p, "HTTP/1.1 200 OK\r\n");
    assert(out.success);
    assert(out.first  == "HTTP/1.1");
    assert(out.second == "200");
    assert(out.third  == "OK");
    assert(out.phase  == phase_t::field_line);
}

void test_first_line_wait_more()
{
    std::cout << "  case: half line -> wait_more" << std::endl;
    parser_t p;
    auto out = do_first_line(p, "GET /index");
    assert(out.wait_more);
    assert(out.consumed == 0);
    assert(out.phase == phase_t::first_line);
}

void test_first_line_limit()
{
    std::cout << "  case: first_line_limit exceeded" << std::endl;
    parser_t p;
    p.set_first_line_limit(10);
    auto out = do_first_line(p, "GET /very/long/path HTTP/1.1\r\n");
    assert(out.error);
    assert(out.consumed == 0);
}

void test_first_line_limit_without_crlf()
{
    std::cout << "  case: no CRLF, length > limit -> error" << std::endl;
    parser_t p;
    p.set_first_line_limit(5);
    auto out = do_first_line(p, "GET /very/long");
    assert(out.error);
    assert(out.consumed == 0);
}

void test_first_line_phase_mismatch()
{
    std::cout << "  case: phase mismatch" << std::endl;
    parser_t p;
    do_first_line(p, "GET / HTTP/1.1\r\n");
    auto out = do_first_line(p, "GET / HTTP/1.1\r\n");
    assert(out.error);
}

void test_first_line_bad_format()
{
    std::cout << "  case: \"GET /\" no version -> bad_protocol" << std::endl;
    parser_t p;
    auto out = do_first_line(p, "GET /\r\n");
    assert(out.error);
}

void test_field_line_normal()
{
    std::cout << "  case: \"Host: example.com\"" << std::endl;
    parser_t p;
    do_first_line(p, "GET / HTTP/1.1\r\n");
    auto out = do_field_line(p, "Host: example.com\r\n");
    assert(out.success);
    assert(out.key   == "Host");
    assert(out.value == "example.com");
    assert(out.phase == phase_t::field_line);
}

void test_field_line_trim()
{
    std::cout << "  case: \"X-Test:   value  \"" << std::endl;
    parser_t p;
    do_first_line(p, "GET / HTTP/1.1\r\n");
    auto out = do_field_line(p, "X-Test:   value  \r\n");
    assert(out.success);
    assert(out.key   == "X-Test");
    assert(out.value == "value  ");
}

void test_field_line_empty_value()
{
    std::cout << "  case: \"X-Empty:\" empty value" << std::endl;
    parser_t p;
    do_first_line(p, "GET / HTTP/1.1\r\n");
    auto out = do_field_line(p, "X-Empty:\r\n");
    assert(out.success);
    assert(out.key   == "X-Empty");
    assert(out.value == "");
}

void test_field_line_empty_ends_header()
{
    std::cout << "  case: empty line -> phase body" << std::endl;
    parser_t p;
    do_first_line(p, "GET / HTTP/1.1\r\n");
    auto out = do_field_line(p, "\r\n");
    assert(out.success);
    assert(out.consumed == 2);
    assert(out.phase == phase_t::body);
}

void test_field_line_wait_more()
{
    std::cout << "  case: \"Host: exa\" -> wait_more" << std::endl;
    parser_t p;
    do_first_line(p, "GET / HTTP/1.1\r\n");
    auto out = do_field_line(p, "Host: exa");
    assert(out.wait_more);
    assert(out.consumed == 0);
}

void test_field_line_limit()
{
    std::cout << "  case: field_line_limit exceeded" << std::endl;
    parser_t p;
    p.set_field_line_limit(10);
    do_first_line(p, "GET / HTTP/1.1\r\n");
    auto out = do_field_line(p, "Host: very-long-value\r\n");
    assert(out.error);
}

void test_field_total_limit()
{
    std::cout << "  case: field_total_limit exceeded" << std::endl;
    parser_t p;
    p.set_field_total_limit(20);
    do_first_line(p, "GET / HTTP/1.1\r\n");
    do_field_line(p, "Host: example.com\r\n");
    auto out = do_field_line(p, "X-Test: abc\r\n");
    assert(out.error);
}

void test_field_line_phase_mismatch()
{
    std::cout << "  case: parse_field_line at phase first_line" << std::endl;
    parser_t p;
    auto out = do_field_line(p, "Host: x\r\n");
    assert(out.error);
}

void test_body_length()
{
    std::cout << "  case: body_remaining 5, push 5 bytes -> complete" << std::endl;
    parser_t p;
    advance_to_body(p);
    p.set_body_remaining(5);

    std::size_t consumed = p.parse_body("hello", 5,
        [](lcy::system::error_code ec, const parser_t::body_view& v, result_t r) {
            assert(r == result_t::success);
            assert(!ec);
            assert(v.body_.length_ == 5);
        });
    assert(consumed == 5);
    assert(p.get_parse_phase() == phase_t::complete);
}

void test_body_length_truncate()
{
    std::cout << "  case: body_remaining 3, push 5 bytes -> consume 3" << std::endl;
    parser_t p;
    advance_to_body(p);
    p.set_body_remaining(3);

    std::size_t consumed = p.parse_body("hello", 5,
        [](lcy::system::error_code, const parser_t::body_view& v, result_t r) {
            assert(r == result_t::success);
            assert(v.body_.length_ == 3);
        });
    assert(consumed == 3);
    assert(p.get_parse_phase() == phase_t::complete);
}

void test_body_phase_mismatch()
{
    std::cout << "  case: parse_body at phase first_line" << std::endl;
    parser_t p;
    std::size_t consumed = p.parse_body("hello", 5,
        [](lcy::system::error_code ec, const parser_t::body_view&, result_t r) {
            assert(r == result_t::error_occurred);
            assert(ec);
        });
    assert(consumed == 0);
}

void test_chunk_one()
{
    std::cout << "  case: chunk \"5\\r\\nhello\\r\\n\"" << std::endl;
    parser_t p;
    advance_to_body(p);
    p.set_chunk_mode(true);

    std::string input = "5\r\nhello\r\n";
    std::size_t total = 0;
    std::string payload;

    while ( p.get_parse_phase() == phase_t::body ) {
        std::size_t n = p.parse_chunk(input.data() + total, input.size() - total,
            [&](lcy::system::error_code, const parser_t::chunk_view& v, result_t r) {
                if ( r == result_t::success && v.payload_.length_ > 0 ) {
                    payload.append(v.payload_.ptr_, v.payload_.length_);
                }
            });
        if ( n == 0 ) { break; }
        total += n;
    }

    assert(payload == "hello");
    assert(total == input.size());
}

void test_chunk_zero_complete()
{
    std::cout << "  case: \"0\\r\\n\" -> complete" << std::endl;
    parser_t p;
    advance_to_body(p);
    p.set_chunk_mode(true);

    std::string input = "0\r\n";
    std::size_t total = 0;
    while ( p.get_parse_phase() == phase_t::body ) {
        std::size_t n = p.parse_chunk(input.data() + total, input.size() - total,
            [](lcy::system::error_code, const parser_t::chunk_view&, result_t) {});
        if ( n == 0 ) { break; }
        total += n;
    }
    assert(p.get_parse_phase() == phase_t::complete);
}

void test_chunk_payload_limit()
{
    std::cout << "  case: chunk size > chunk_payload_limit" << std::endl;
    parser_t p;
    p.set_chunk_payload_limit(4);
    advance_to_body(p);
    p.set_chunk_mode(true);

    std::string input = "10\r\n";
    p.parse_chunk(input.data(), input.size(),
        [](lcy::system::error_code ec, const parser_t::chunk_view&, result_t r) {
            assert(r == result_t::error_occurred);
            assert(ec);
        });
}

void test_chunk_line_limit()
{
    std::cout << "  case: chunk line > chunk_line_limit" << std::endl;
    parser_t p;
    p.set_chunk_line_limit(4);
    advance_to_body(p);
    p.set_chunk_mode(true);

    std::string input = "12345678";
    p.parse_chunk(input.data(), input.size(),
        [](lcy::system::error_code ec, const parser_t::chunk_view&, result_t r) {
            assert(r == result_t::error_occurred);
            assert(ec);
        });
}

void test_chunk_wait_more()
{
    std::cout << "  case: \"5\\r\\nhel\" -> wait_more" << std::endl;
    parser_t p;
    advance_to_body(p);
    p.set_chunk_mode(true);

    std::string input = "5\r\nhel";
    std::size_t total = 0;
    bool got_wait_more = false;
    while ( p.get_parse_phase() == phase_t::body ) {
        std::size_t n = p.parse_chunk(input.data() + total, input.size() - total,
            [&](lcy::system::error_code, const parser_t::chunk_view&, result_t r) {
                if ( r == result_t::wait_more ) { got_wait_more = true; }
            });
        if ( n == 0 ) { break; }
        total += n;
    }
    assert(got_wait_more);
    assert(p.get_parse_phase() == phase_t::body);
}

void test_trailer()
{
    std::cout << "  case: trailer field then empty line -> complete" << std::endl;
    parser_t p;
    advance_to_body(p);
    p.set_chunk_mode(true);
    p.set_trailer_check(true);

    std::string input = "0\r\nX-Trailer: abc\r\n\r\n";
    std::size_t total = 0;
    std::string trailer_key, trailer_value;

    while ( p.get_parse_phase() != phase_t::complete ) {
        if ( p.get_parse_phase() == phase_t::body ) {
            total += p.parse_chunk(input.data() + total, input.size() - total,
                [](lcy::system::error_code, const parser_t::chunk_view&, result_t) {});
        } else if ( p.get_parse_phase() == phase_t::trailer ) {
            total += p.parse_trailer(input.data() + total, input.size() - total,
                [&](lcy::system::error_code, const parser_t::field_line_view& v, result_t r) {
                    if ( r == result_t::success && v.key_.length_ > 0 ) {
                        trailer_key   = to_string(v.key_);
                        trailer_value = to_string(v.value_);
                    }
                });
        } else {
            break;
        }
    }

    assert(p.get_parse_phase() == phase_t::complete);
    assert(trailer_key == "X-Trailer");
    assert(trailer_value == "abc");
}

void test_trailer_phase_mismatch()
{
    std::cout << "  case: parse_trailer at phase body" << std::endl;
    parser_t p;
    advance_to_body(p);
    p.parse_trailer("X: y\r\n", 6,
        [](lcy::system::error_code ec, const parser_t::field_line_view&, result_t r) {
            assert(r == result_t::error_occurred);
            assert(ec);
        });
}

void test_reset()
{
    std::cout << "  case: reset clears phase, keeps limits" << std::endl;
    parser_t p;
    p.set_first_line_limit(100);
    do_first_line(p, "GET / HTTP/1.1\r\n");
    assert(p.get_parse_phase() == phase_t::field_line);

    p.reset();
    assert(p.get_parse_phase() == phase_t::first_line);
    assert(p.get_parse_bytes() == 0);

    auto out = do_first_line(p, std::string(200, 'a') + "\r\n");
    assert(out.error);
}

void test_split_input_first_line()
{
    std::cout << "  case: feed \"GET / HT\" then \"TP/1.1\\r\\n\"" << std::endl;
    parser_t p;
    auto out = do_first_line(p, "GET / HT");
    assert(out.wait_more);

    out = do_first_line(p, "GET / HTTP/1.1\r\n");
    assert(out.success);
    assert(out.third == "HTTP/1.1");
}

void test_full_header()
{
    std::cout << "  case: full header sequence" << std::endl;
    parser_t p;

    auto fl = do_first_line(p, "POST /submit HTTP/1.1\r\n");
    assert(fl.success);
    assert(fl.first == "POST");

    auto f1 = do_field_line(p, "Host: example.com\r\n");
    assert(f1.success);
    assert(f1.key == "Host");

    auto f2 = do_field_line(p, "Content-Length: 5\r\n");
    assert(f2.success);
    assert(f2.key == "Content-Length");
    assert(f2.value == "5");

    auto f3 = do_field_line(p, "\r\n");
    assert(f3.success);
    assert(p.get_parse_phase() == phase_t::body);
}

void test_full_request_no_body()
{
    std::cout << "  case: GET / HTTP/1.1 + Host + empty line -> complete" << std::endl;
    parser_t p;

    auto fl = do_first_line(p, "GET / HTTP/1.1\r\n");
    assert(fl.success);
    assert(fl.first == "GET");
    assert(fl.second == "/");
    assert(fl.third == "HTTP/1.1");

    auto f1 = do_field_line(p, "Host: example.com\r\n");
    assert(f1.success);
    assert(f1.key == "Host");
    assert(f1.value == "example.com");

    auto f2 = do_field_line(p, "\r\n");
    assert(f2.success);
    assert(p.get_parse_phase() == phase_t::body);

    p.set_body_remaining(0);
    std::size_t consumed = p.parse_body("", 0,
        [](lcy::system::error_code, const parser_t::body_view&, result_t r) {
            assert(r == result_t::wait_more);
        });
    assert(consumed == 0);
}

void test_full_request_with_body()
{
    std::cout << "  case: POST /submit + Content-Length: 5 + hello" << std::endl;
    parser_t p;

    auto fl = do_first_line(p, "POST /submit HTTP/1.1\r\n");
    assert(fl.success);
    assert(fl.first == "POST");

    auto f1 = do_field_line(p, "Host: example.com\r\n");
    assert(f1.success);

    auto f2 = do_field_line(p, "Content-Length: 5\r\n");
    assert(f2.success);
    assert(f2.value == "5");

    auto f3 = do_field_line(p, "\r\n");
    assert(f3.success);
    assert(p.get_parse_phase() == phase_t::body);

    p.set_body_remaining(5);

    std::string body;
    std::size_t consumed = p.parse_body("hello", 5,
        [&](lcy::system::error_code ec, const parser_t::body_view& v, result_t r) {
            assert(!ec);
            assert(r == result_t::success);
            body.append(v.body_.ptr_, v.body_.length_);
        });
    assert(consumed == 5);
    assert(body == "hello");
    assert(p.get_parse_phase() == phase_t::complete);
}

void test_full_request_chunked()
{
    std::cout << "  case: POST chunked \"5\\r\\nhello\\r\\n6\\r\\n world\\r\\n0\\r\\n\\r\\n\"" << std::endl;
    parser_t p;

    auto fl = do_first_line(p, "POST / HTTP/1.1\r\n");
    assert(fl.success);

    auto f1 = do_field_line(p, "Transfer-Encoding: chunked\r\n");
    assert(f1.success);
    assert(f1.value == "chunked");

    auto f2 = do_field_line(p, "\r\n");
    assert(f2.success);
    assert(p.get_parse_phase() == phase_t::body);

    p.set_chunk_mode(true);

    std::string input = "5\r\nhello\r\n6\r\n world\r\n0\r\n\r\n";
    std::size_t total = 0;
    std::string payload;

    while ( p.get_parse_phase() == phase_t::body ) {
        std::size_t n = p.parse_chunk(input.data() + total, input.size() - total,
            [&](lcy::system::error_code, const parser_t::chunk_view& v, result_t r) {
                if ( r == result_t::success && v.payload_.length_ > 0 ) {
                    payload.append(v.payload_.ptr_, v.payload_.length_);
                }
            });
        if ( n == 0 ) { break; }
        total += n;
    }

    assert(payload == "hello world");
    assert(p.get_parse_phase() == phase_t::complete);
}

void test_full_request_chunked_trailer()
{
    std::cout << "  case: chunked + trailer \"0\\r\\nX-Check: abc\\r\\n\\r\\n\"" << std::endl;
    parser_t p;

    do_first_line(p, "POST / HTTP/1.1\r\n");
    do_field_line(p, "Transfer-Encoding: chunked\r\n");
    do_field_line(p, "\r\n");
    p.set_chunk_mode(true);
    p.set_trailer_check(true);

    std::string input = "0\r\nX-Check: abc\r\n\r\n";
    std::size_t total = 0;
    std::string trailer_key, trailer_value;

    while ( p.get_parse_phase() != phase_t::complete ) {
        if ( p.get_parse_phase() == phase_t::body ) {
            total += p.parse_chunk(input.data() + total, input.size() - total,
                [](lcy::system::error_code, const parser_t::chunk_view&, result_t) {});
        } else if ( p.get_parse_phase() == phase_t::trailer ) {
            total += p.parse_trailer(input.data() + total, input.size() - total,
                [&](lcy::system::error_code, const parser_t::field_line_view& v, result_t r) {
                    if ( r == result_t::success && v.key_.length_ > 0 ) {
                        trailer_key   = to_string(v.key_);
                        trailer_value = to_string(v.value_);
                    }
                });
        } else {
            break;
        }
    }

    assert(p.get_parse_phase() == phase_t::complete);
    assert(trailer_key == "X-Check");
    assert(trailer_value == "abc");
}

void test_full_request_split_feed()
{
    std::cout << "  case: feed full request in pieces" << std::endl;
    parser_t p;

    auto out = do_first_line(p, "POST /sub");
    assert(out.wait_more);

    out = do_first_line(p, "POST /submit HTTP/1.1\r\n");
    assert(out.success);
    assert(out.first == "POST");
    assert(out.second == "/submit");

    auto f1 = do_field_line(p, "Host: example.com\r\n");
    assert(f1.success);
    assert(f1.key == "Host");

    auto f2 = do_field_line(p, "Content-Le");
    assert(f2.wait_more);

    f2 = do_field_line(p, "Content-Length: 3\r\n");
    assert(f2.success);
    assert(f2.key == "Content-Length");
    assert(f2.value == "3");

    auto f3 = do_field_line(p, "\r\n");
    assert(f3.success);
    assert(p.get_parse_phase() == phase_t::body);

    p.set_body_remaining(3);
    std::string body;
    std::size_t consumed = p.parse_body("abc", 3,
        [&](lcy::system::error_code, const parser_t::body_view& v, result_t r) {
            assert(r == result_t::success);
            body.append(v.body_.ptr_, v.body_.length_);
        });
    assert(consumed == 3);
    assert(body == "abc");
    assert(p.get_parse_phase() == phase_t::complete);
}

void test_full_response_with_body()
{
    std::cout << "  case: HTTP/1.1 200 OK + Content-Length: 2 + ok" << std::endl;
    parser_t p;

    auto fl = do_first_line(p, "HTTP/1.1 200 OK\r\n");
    assert(fl.success);
    assert(fl.first == "HTTP/1.1");
    assert(fl.second == "200");
    assert(fl.third == "OK");

    auto f1 = do_field_line(p, "Content-Length: 2\r\n");
    assert(f1.success);

    auto f2 = do_field_line(p, "\r\n");
    assert(f2.success);
    assert(p.get_parse_phase() == phase_t::body);

    p.set_body_remaining(2);
    std::string body;
    std::size_t consumed = p.parse_body("ok", 2,
        [&](lcy::system::error_code, const parser_t::body_view& v, result_t r) {
            assert(r == result_t::success);
            body.append(v.body_.ptr_, v.body_.length_);
        });
    assert(consumed == 2);
    assert(body == "ok");
    assert(p.get_parse_phase() == phase_t::complete);
}

void test_full_response_chunked_trailer()
{
    std::cout << "  case: 200 chunked + trailer full flow" << std::endl;
    parser_t p;

    do_first_line(p, "HTTP/1.1 200 OK\r\n");
    do_field_line(p, "Transfer-Encoding: chunked\r\n");
    do_field_line(p, "\r\n");
    p.set_chunk_mode(true);
    p.set_trailer_check(true);

    std::string input = "4\r\nWiki\r\n5\r\npedia\r\n0\r\nX-Sum: xyz\r\n\r\n";
    std::size_t total = 0;
    std::string payload, trailer_key, trailer_value;

    while ( p.get_parse_phase() != phase_t::complete ) {
        if ( p.get_parse_phase() == phase_t::body ) {
            total += p.parse_chunk(input.data() + total, input.size() - total,
                [&](lcy::system::error_code, const parser_t::chunk_view& v, result_t r) {
                    if ( r == result_t::success && v.payload_.length_ > 0 ) {
                        payload.append(v.payload_.ptr_, v.payload_.length_);
                    }
                });
        } else if ( p.get_parse_phase() == phase_t::trailer ) {
            total += p.parse_trailer(input.data() + total, input.size() - total,
                [&](lcy::system::error_code, const parser_t::field_line_view& v, result_t r) {
                    if ( r == result_t::success && v.key_.length_ > 0 ) {
                        trailer_key   = to_string(v.key_);
                        trailer_value = to_string(v.value_);
                    }
                });
        } else {
            break;
        }
    }

    assert(p.get_parse_phase() == phase_t::complete);
    assert(payload == "Wikipedia");
    assert(trailer_key == "X-Sum");
    assert(trailer_value == "xyz");
}

void test_full_request_then_reset()
{
    std::cout << "  case: parse full request, reset, parse another" << std::endl;
    parser_t p;

    do_first_line(p, "GET /first HTTP/1.1\r\n");
    do_field_line(p, "\r\n");
    p.set_body_remaining(0);
    p.reset();

    assert(p.get_parse_phase() == phase_t::first_line);
    assert(p.get_parse_bytes() == 0);

    auto fl = do_first_line(p, "GET /second HTTP/1.1\r\n");
    assert(fl.success);
    assert(fl.second == "/second");

    auto f1 = do_field_line(p, "\r\n");
    assert(f1.success);
    assert(p.get_parse_phase() == phase_t::body);
}

bool is_http_error(const lcy::system::error_code& ec,
                   lcy::bytra::http::error expected)
{
    return ec == static_cast<int>(expected);
}

void test_error_bad_protocol_first_line()
{
    std::cout << "  case: bad_protocol from bad first line" << std::endl;
    parser_t p;
    auto out = do_first_line(p, "GET /\r\n");
    assert(out.error);
    assert(is_http_error(out.ec, lcy::bytra::http::error::bad_protocol));
}

void test_error_bad_protocol_field_line()
{
    std::cout << "  case: bad_protocol from field line without colon" << std::endl;
    parser_t p;
    do_first_line(p, "GET / HTTP/1.1\r\n");
    auto out = do_field_line(p, "InvalidFieldLine\r\n");
    assert(out.error);
    assert(is_http_error(out.ec, lcy::bytra::http::error::bad_protocol));
}

void test_error_phase_mismatch_first_line()
{
    std::cout << "  case: phase_mismatch from parse_first_line" << std::endl;
    parser_t p;
    do_first_line(p, "GET / HTTP/1.1\r\n");
    auto out = do_first_line(p, "GET / HTTP/1.1\r\n");
    assert(out.error);
    assert(is_http_error(out.ec, lcy::bytra::http::error::phase_mismatch));
}

void test_error_phase_mismatch_field_line()
{
    std::cout << "  case: phase_mismatch from parse_field_line" << std::endl;
    parser_t p;
    auto out = do_field_line(p, "Host: x\r\n");
    assert(out.error);
    assert(is_http_error(out.ec, lcy::bytra::http::error::phase_mismatch));
}

void test_error_phase_mismatch_body()
{
    std::cout << "  case: phase_mismatch from parse_body" << std::endl;
    parser_t p;
    lcy::system::error_code captured;
    p.parse_body("hello", 5,
        [&](lcy::system::error_code ec, const parser_t::body_view&, result_t r) {
            assert(r == result_t::error_occurred);
            captured = ec;
        });
    assert(is_http_error(captured, lcy::bytra::http::error::phase_mismatch));
}

void test_error_phase_mismatch_trailer()
{
    std::cout << "  case: phase_mismatch from parse_trailer" << std::endl;
    parser_t p;
    advance_to_body(p);
    lcy::system::error_code captured;
    p.parse_trailer("X: y\r\n", 6,
        [&](lcy::system::error_code ec, const parser_t::field_line_view&, result_t r) {
            assert(r == result_t::error_occurred);
            captured = ec;
        });
    assert(is_http_error(captured, lcy::bytra::http::error::phase_mismatch));
}

void test_error_phase_mismatch_chunk()
{
    std::cout << "  case: phase_mismatch from parse_chunk" << std::endl;
    parser_t p;
    lcy::system::error_code captured;
    p.parse_chunk("5\r\nhello\r\n", 11,
        [&](lcy::system::error_code ec, const parser_t::chunk_view&, result_t r) {
            assert(r == result_t::error_occurred);
            captured = ec;
        });
    assert(is_http_error(captured, lcy::bytra::http::error::phase_mismatch));
}

void test_error_first_line_limit_with_crlf()
{
    std::cout << "  case: first_line_limit with CRLF" << std::endl;
    parser_t p;
    p.set_first_line_limit(10);
    auto out = do_first_line(p, "GET /very/long/path HTTP/1.1\r\n");
    assert(out.error);
    assert(is_http_error(out.ec, lcy::bytra::http::error::first_line_limit));
}

void test_error_first_line_limit_without_crlf()
{
    std::cout << "  case: first_line_limit without CRLF" << std::endl;
    parser_t p;
    p.set_first_line_limit(5);
    auto out = do_first_line(p, "GET /very/long");
    assert(out.error);
    assert(is_http_error(out.ec, lcy::bytra::http::error::first_line_limit));
}

void test_error_field_line_limit_with_crlf()
{
    std::cout << "  case: field_line_limit with CRLF" << std::endl;
    parser_t p;
    p.set_field_line_limit(10);
    do_first_line(p, "GET / HTTP/1.1\r\n");
    auto out = do_field_line(p, "Host: very-long-value\r\n");
    assert(out.error);
    assert(is_http_error(out.ec, lcy::bytra::http::error::field_line_limit));
}

void test_error_field_line_limit_without_crlf()
{
    std::cout << "  case: field_line_limit without CRLF" << std::endl;
    parser_t p;
    p.set_field_line_limit(10);
    do_first_line(p, "GET / HTTP/1.1\r\n");
    auto out = do_field_line(p, "Host: very-long-value-no-crlf");
    assert(out.error);
    assert(is_http_error(out.ec, lcy::bytra::http::error::field_line_limit));
}

void test_error_field_total_limit()
{
    std::cout << "  case: field_total_limit" << std::endl;
    parser_t p;
    p.set_field_total_limit(20);
    do_first_line(p, "GET / HTTP/1.1\r\n");
    do_field_line(p, "Host: example.com\r\n");
    auto out = do_field_line(p, "X-Test: abc\r\n");
    assert(out.error);
    assert(is_http_error(out.ec, lcy::bytra::http::error::field_total_limit));
}

void test_error_chunk_payload_limit()
{
    std::cout << "  case: chunk_payload_limit" << std::endl;
    parser_t p;
    p.set_chunk_payload_limit(4);
    advance_to_body(p);
    p.set_chunk_mode(true);

    lcy::system::error_code captured;
    p.parse_chunk("10\r\n", 4,
        [&](lcy::system::error_code ec, const parser_t::chunk_view&, result_t r) {
            assert(r == result_t::error_occurred);
            captured = ec;
        });
    assert(is_http_error(captured, lcy::bytra::http::error::chunk_payload_limit));
}

void test_error_body_total_limit_length()
{
    std::cout << "  case: body_total_limit from parse_body" << std::endl;
    parser_t p;
    advance_to_body(p);
    p.set_body_total_limit(3);
    p.set_body_remaining(10);

    lcy::system::error_code captured;
    p.parse_body("hello", 5,
        [&](lcy::system::error_code ec, const parser_t::body_view&, result_t r) {
            assert(r == result_t::error_occurred);
            captured = ec;
        });
    assert(is_http_error(captured, lcy::bytra::http::error::body_total_limit));
}

void test_error_body_total_limit_chunked()
{
    std::cout << "  case: body_total_limit from parse_chunk" << std::endl;
    parser_t p;
    advance_to_body(p);
    p.set_chunk_mode(true);
    p.set_body_total_limit(3);

    lcy::system::error_code captured;
    p.parse_chunk("5\r\nhello", 8,
        [&](lcy::system::error_code ec, const parser_t::chunk_view&, result_t r) {
            if ( r == result_t::error_occurred ) {
                captured = ec;
            }
        });
    assert(is_http_error(captured, lcy::bytra::http::error::body_total_limit));
}

void test_error_success_empty_ec()
{
    std::cout << "  case: success path ec is empty" << std::endl;
    parser_t p;
    auto out = do_first_line(p, "GET / HTTP/1.1\r\n");
    assert(out.success);
    assert(!out.ec);
}

} // namespace

int main()
{
    std::cout << "===== stream_parser tests =====" << std::endl;

    RUN_TEST(test_first_line_request);
    RUN_TEST(test_first_line_response);
    RUN_TEST(test_first_line_wait_more);
    RUN_TEST(test_first_line_limit);
    RUN_TEST(test_first_line_limit_without_crlf);
    RUN_TEST(test_first_line_phase_mismatch);
    RUN_TEST(test_first_line_bad_format);
    RUN_TEST(test_field_line_normal);
    RUN_TEST(test_field_line_trim);
    RUN_TEST(test_field_line_empty_value);
    RUN_TEST(test_field_line_empty_ends_header);
    RUN_TEST(test_field_line_wait_more);
    RUN_TEST(test_field_line_limit);
    RUN_TEST(test_field_total_limit);
    RUN_TEST(test_field_line_phase_mismatch);
    RUN_TEST(test_body_length);
    RUN_TEST(test_body_length_truncate);
    RUN_TEST(test_body_phase_mismatch);
    RUN_TEST(test_chunk_one);
    RUN_TEST(test_chunk_zero_complete);
    RUN_TEST(test_chunk_payload_limit);
    RUN_TEST(test_chunk_line_limit);
    RUN_TEST(test_chunk_wait_more);
    RUN_TEST(test_trailer);
    RUN_TEST(test_trailer_phase_mismatch);
    RUN_TEST(test_reset);
    RUN_TEST(test_split_input_first_line);
    RUN_TEST(test_full_header);
    RUN_TEST(test_full_request_no_body);
    RUN_TEST(test_full_request_with_body);
    RUN_TEST(test_full_request_chunked);
    RUN_TEST(test_full_request_chunked_trailer);
    RUN_TEST(test_full_request_split_feed);
    RUN_TEST(test_full_response_with_body);
    RUN_TEST(test_full_response_chunked_trailer);
    RUN_TEST(test_full_request_then_reset);
    RUN_TEST(test_error_bad_protocol_first_line);
    RUN_TEST(test_error_bad_protocol_field_line);
    RUN_TEST(test_error_phase_mismatch_first_line);
    RUN_TEST(test_error_phase_mismatch_field_line);
    RUN_TEST(test_error_phase_mismatch_body);
    RUN_TEST(test_error_phase_mismatch_trailer);
    RUN_TEST(test_error_phase_mismatch_chunk);
    RUN_TEST(test_error_first_line_limit_with_crlf);
    RUN_TEST(test_error_first_line_limit_without_crlf);
    RUN_TEST(test_error_field_line_limit_with_crlf);
    RUN_TEST(test_error_field_line_limit_without_crlf);
    RUN_TEST(test_error_field_total_limit);
    RUN_TEST(test_error_chunk_payload_limit);
    RUN_TEST(test_error_body_total_limit_length);
    RUN_TEST(test_error_body_total_limit_chunked);
    RUN_TEST(test_error_success_empty_ec);

    std::cout << "===== " << g_passed << " passed, "
              << g_failed << " failed =====" << std::endl;
    return g_failed == 0 ? 0 : 1;
}