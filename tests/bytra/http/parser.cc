#include "lcy/bytra/flat_buffer.hpp"
#include "lcy/bytra/http/request_parser.hpp"
#include "lcy/bytra/http/response_parser.hpp"
#include "lcy/bytra/http/request.hpp"
#include "lcy/bytra/http/response.hpp"
#include "lcy/bytra/http/method.hpp"
#include "lcy/bytra/http/status.hpp"
#include "lcy/bytra/http/version.hpp"
#include "lcy/bytra/http/error.hpp"
#include "lcy/bytra/http/detail/error.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

using namespace lcy::bytra;
using namespace lcy::bytra::http;
using lcy::bytra::http::detail::make_http_error_code;

static void expect_true(bool cond, const char* msg)
{
    if ( !cond ) {
        std::cerr << "FAIL: " << msg << "\n";
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

static void expect_str(const std::string& got, const std::string& want,
                       const char* msg)
{
    if ( got != want ) {
        std::cerr << "FAIL: " << msg << "\n  got : [" << got
                  << "]\n  want: [" << want << "]\n";
        std::abort();
    }
}

static void expect_ec(const lcy::system::error_code& ec,
                      lcy::bytra::http::error want, const char* msg)
{
    if ( ec != static_cast<int>(want) ) {
        std::cerr << "FAIL: " << msg << "\n  got ec=" << ec
                  << "\n  want=" << static_cast<int>(want) << "\n";
        std::abort();
    }
}

static std::string buf_to_string(const flat_buffer& b)
{
    return std::string(static_cast<const char*>(b.data()), b.size());
}

template <typename Parser, typename Message>
static lcy::system::error_code feed_all(Parser& p, Message& m,
                                        const std::string& s)
{
    lcy::system::error_code ec;
    p.parse(s.data(), s.size(), m, ec);
    return ec;
}

template <typename Parser, typename Message>
static lcy::system::error_code feed_pieces(Parser& p, Message& m,
    const std::vector<std::string>& pieces)
{
    flat_buffer buf;
    lcy::system::error_code ec;
    std::size_t last = 0;

    for ( const auto& piece : pieces ) {
        auto view = buf.prepare(piece.size());
        std::memcpy(view.data(), piece.data(), piece.size());
        buf.commit(piece.size());

        p.parse(static_cast<const char*>(buf.data()), buf.size(), m, ec);

        bool is_wait_more = ( ec ==
            static_cast<int>(lcy::bytra::http::error::wait_more) );
        if ( ec && !is_wait_more ) {
            return ec;
        }

        std::size_t now = p.get_parsed_bytes();
        buf.consume(now - last);
        last = now;
    }
    return ec;
}

static void test_request_minimal_get()
{
    std::cout << "[test_request_minimal_get]\n";
    request_parser p;
    request req;
    auto ec = feed_all(p, req, "GET / HTTP/1.1\r\n\r\n");
    expect_true(!ec, "ec success");
    expect_true(req.get_method() == method::get, "method get");
    expect_str(req.get_target(), "/", "target /");
    expect_true(req.get_version() == version::http_1_1, "version 1.1");
    expect_size(req.body_container().size(), 0, "empty body");
    std::cout << "  ok\n";
}

static void test_request_with_headers()
{
    std::cout << "[test_request_with_headers]\n";
    request_parser p;
    request req;
    auto ec = feed_all(p, req,
        "GET /path HTTP/1.1\r\nHost: a\r\n\r\n");
    expect_true(!ec, "ec success");
    expect_str(req.get_target(), "/path", "target /path");
    expect_size(req.count("Host"), 1, "Host count");
    auto it = req.get_field("Host");
    expect_true(it != req.field_end(), "Host found");
    expect_str(it->second, "a", "Host value");
    std::cout << "  ok\n";
}

static void test_request_post_content_length()
{
    std::cout << "[test_request_post_content_length]\n";
    request_parser p;
    request req;
    auto ec = feed_all(p, req,
        "POST / HTTP/1.1\r\nContent-Length: 5\r\n\r\nhello");
    expect_true(!ec, "ec success");
    expect_true(req.get_method() == method::post, "method post");
    expect_size(req.body_container().size(), 5, "body size 5");
    expect_str(buf_to_string(req.body_container()), "hello", "body hello");
    std::cout << "  ok\n";
}

static void test_request_chunked()
{
    std::cout << "[test_request_chunked]\n";
    request_parser p;
    request req;
    auto ec = feed_all(p, req,
        "POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n"
        "5\r\nhello\r\n0\r\n\r\n");
    expect_true(!ec, "ec success");
    expect_size(req.body_container().size(), 5, "body size 5");
    expect_str(buf_to_string(req.body_container()), "hello", "body hello");
    std::cout << "  ok\n";
}

static void test_request_chunked_trailer()
{
    std::cout << "[test_request_chunked_trailer]\n";
    request_parser p;
    request req;
    auto ec = feed_all(p, req,
        "POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n"
        "5\r\nhello\r\n0\r\nX-Trailer: v\r\n\r\n");
    expect_true(!ec, "ec success");
    expect_size(req.body_container().size(), 5, "body size 5");
    auto it = req.get_field("X-Trailer");
    expect_true(it != req.field_end(), "trailer found");
    expect_str(it->second, "v", "trailer value");
    std::cout << "  ok\n";
}

static void test_trailer_multiple_fields()
{
    std::cout << "[test_trailer_multiple_fields]\n";
    request_parser p;
    request req;
    auto ec = feed_all(p, req,
        "POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n"
        "5\r\nhello\r\n0\r\nX-A: 1\r\nX-B: 2\r\n\r\n");
    expect_true(!ec, "success");
    auto a = req.get_field("X-A");
    expect_true(a != req.field_end(), "X-A found");
    expect_str(a->second, "1", "X-A value");
    auto b = req.get_field("X-B");
    expect_true(b != req.field_end(), "X-B found");
    expect_str(b->second, "2", "X-B value");
    std::cout << "  ok\n";
}

static void test_trailer_same_name_as_header()
{
    std::cout << "[test_trailer_same_name_as_header]\n";
    request_parser p;
    request req;
    auto ec = feed_all(p, req,
        "POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\nX-T: h\r\n\r\n"
        "5\r\nhello\r\n0\r\nX-T: t\r\n\r\n");
    expect_true(!ec, "success");
    expect_size(req.count("X-T"), 2, "X-T count 2");
    std::vector<std::string> vals;
    for (auto it = req.field_begin(); it != req.field_end(); ++it) {
        if (it->first == "X-T") vals.push_back(it->second);
    }
    expect_size(vals.size(), 2, "two X-T");
    expect_str(vals[0], "h", "header X-T first");
    expect_str(vals[1], "t", "trailer X-T second");
    std::cout << "  ok\n";
}

static void test_trailer_name_split()
{
    std::cout << "[test_trailer_name_split]\n";
    request_parser p;
    request req;
    auto ec = feed_pieces(p, req,
        {"POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n"
         "5\r\nhello\r\n0\r\nX-Traile",
         "r: v\r\n\r\n"});
    expect_true(!ec, "success");
    auto it = req.get_field("X-Trailer");
    expect_true(it != req.field_end(), "X-Trailer found");
    expect_str(it->second, "v", "X-Trailer value");
    std::cout << "  ok\n";
}

static void test_trailer_done_after_empty_line()
{
    std::cout << "[test_trailer_done_after_empty_line]\n";
    request_parser p;
    request req;
    lcy::system::error_code ec;
    const char* s =
        "POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n"
        "5\r\nhello\r\n0\r\nX-T: v\r\n\r\n";
    p.parse(s, std::strlen(s), req, ec);
    expect_true(!ec, "success");
    // done 后 parse_body 应 invalid_state
    p.parse_body("x", 1, req, ec);
    expect_ec(ec, lcy::bytra::http::error::invalid_state, "done -> invalid_state");
    std::cout << "  ok\n";
}

static void test_trailer_empty()
{
    std::cout << "[test_trailer_empty]\n";
    request_parser p;
    request req;
    auto ec = feed_all(p, req,
        "POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n"
        "5\r\nhello\r\n0\r\n\r\n");
    expect_true(!ec, "success");
    expect_size(req.body_container().size(), 5, "body 5");
    std::cout << "  ok\n";
}

static void test_response_chunked_trailer()
{
    std::cout << "[test_response_chunked_trailer]\n";
    response_parser p;
    response res;
    auto ec = feed_all(p, res,
        "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n"
        "3\r\nabc\r\n0\r\nX-T: v\r\n\r\n");
    expect_true(!ec, "success");
    expect_str(buf_to_string(res.body_container()), "abc", "body abc");
    auto it = res.get_field("X-T");
    expect_true(it != res.field_end(), "X-T found");
    expect_str(it->second, "v", "X-T value");
    std::cout << "  ok\n";
}

static void test_trailer_value_with_colon_ows()
{
    std::cout << "[test_trailer_value_with_colon_ows]\n";
    request_parser p;
    request req;
    auto ec = feed_all(p, req,
        "POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n"
        "5\r\nhello\r\n0\r\nX-T:   a:b:c   \r\n\r\n");
    expect_true(!ec, "success");
    auto it = req.get_field("X-T");
    expect_true(it != req.field_end(), "X-T found");
    expect_str(it->second, "a:b:c", "value trimmed, colon kept");
    std::cout << "  ok\n";
}

static void test_request_chunk_extension()
{
    std::cout << "[test_request_chunk_extension]\n";
    request_parser p;
    request req;
    auto ec = feed_all(p, req,
        "POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n"
        "5;name=value\r\nhello\r\n0\r\n\r\n");
    expect_true(!ec, "ec success");
    expect_size(req.body_container().size(), 5, "body size 5");
    expect_str(buf_to_string(req.body_container()), "hello", "body hello");
    std::cout << "  ok\n";
}

static void test_request_multiple_chunks()
{
    std::cout << "[test_request_multiple_chunks]\n";
    request_parser p;
    request req;
    auto ec = feed_all(p, req,
        "POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n"
        "3\r\nabc\r\n2\r\nde\r\n0\r\n\r\n");
    expect_true(!ec, "ec success");
    expect_size(req.body_container().size(), 5, "body size 5");
    expect_str(buf_to_string(req.body_container()), "abcde", "body abcde");
    std::cout << "  ok\n";
}

static void test_request_http10()
{
    std::cout << "[test_request_http10]\n";
    request_parser p;
    request req;
    auto ec = feed_all(p, req, "GET / HTTP/1.0\r\n\r\n");
    expect_true(!ec, "ec success");
    expect_true(req.get_version() == version::http_1_0, "version 1.0");
    std::cout << "  ok\n";
}

static void test_response_minimal_200()
{
    std::cout << "[test_response_minimal_200]\n";
    response_parser p;
    response res;
    auto ec = feed_all(p, res, "HTTP/1.1 200 OK\r\n\r\n");
    expect_true(!ec, "ec success");
    expect_true(res.get_status() == status::ok, "status ok");
    expect_str(res.get_reason(), "OK", "reason OK");
    std::cout << "  ok\n";
}

static void test_response_no_reason()
{
    std::cout << "[test_response_no_reason]\n";
    response_parser p;
    response res;
    auto ec = feed_all(p, res, "HTTP/1.1 200\r\n\r\n");
    expect_true(!ec, "ec success");
    expect_true(res.get_status() == status::ok, "status ok");
    expect_str(res.get_reason(), "", "reason empty");
    std::cout << "  ok\n";
}

static void test_response_with_body()
{
    std::cout << "[test_response_with_body]\n";
    response_parser p;
    response res;
    auto ec = feed_all(p, res,
        "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nhi");
    expect_true(!ec, "ec success");
    expect_size(res.body_container().size(), 2, "body size 2");
    expect_str(buf_to_string(res.body_container()), "hi", "body hi");
    std::cout << "  ok\n";
}

static void test_response_chunked()
{
    std::cout << "[test_response_chunked]\n";
    response_parser p;
    response res;
    auto ec = feed_all(p, res,
        "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n"
        "5\r\nhello\r\n0\r\n\r\n");
    expect_true(!ec, "ec success");
    expect_size(res.body_container().size(), 5, "body size 5");
    std::cout << "  ok\n";
}

static void test_response_unknown_status_rejected()
{
    std::cout << "[test_response_unknown_status_rejected]\n";
    response_parser p;
    response res;
    auto ec = feed_all(p, res, "HTTP/1.1 299 X\r\n\r\n");
    expect_ec(ec, lcy::bytra::http::error::invalid_header, "299 rejected");
    std::cout << "  ok\n";
}

static void test_split_first_line()
{
    std::cout << "[test_split_first_line]\n";
    request_parser p;
    request req;
    auto ec = feed_pieces(p, req, {"GET / HTT", "P/1.1\r\n\r\n"});
    expect_true(!ec, "ec success");
    expect_true(req.get_method() == method::get, "method get");
    expect_str(req.get_target(), "/", "target /");
    expect_true(req.get_version() == version::http_1_1, "version 1.1");
    std::cout << "  ok\n";
}

static void test_split_crlf()
{
    std::cout << "[test_split_crlf]\n";
    request_parser p;
    request req;
    auto ec = feed_pieces(p, req, {"GET / HTTP/1.1\r", "\n\r\n"});
    expect_true(!ec, "ec success");
    expect_true(req.get_version() == version::http_1_1, "version 1.1");
    std::cout << "  ok\n";
}

static void test_split_header_line()
{
    std::cout << "[test_split_header_line]\n";
    request_parser p;
    request req;
    auto ec = feed_pieces(p, req,
        {"GET / HTTP/1.1\r\nHost: exa", "mple.com\r\n\r\n"});
    expect_true(!ec, "ec success");
    auto it = req.get_field("Host");
    expect_true(it != req.field_end(), "Host found");
    expect_str(it->second, "example.com", "Host value");
    std::cout << "  ok\n";
}

static void test_split_name_value()
{
    std::cout << "[test_split_name_value]\n";
    request_parser p;
    request req;
    auto ec = feed_pieces(p, req,
        {"POST / HTTP/1.1\r\nContent-Le", "ngth: 5\r\n\r\nhello"});
    expect_true(!ec, "ec success");
    expect_size(req.body_container().size(), 5, "body size 5");
    std::cout << "  ok\n";
}

static void test_split_header_end()
{
    std::cout << "[test_split_header_end]\n";
    request_parser p;
    request req;
    auto ec = feed_pieces(p, req,
        {"GET / HTTP/1.1\r\nHost: a\r\n", "\r\n"});
    expect_true(!ec, "ec success");
    std::cout << "  ok\n";
}

static void test_split_body_byte_by_byte()
{
    std::cout << "[test_split_body_byte_by_byte]\n";
    request_parser p;
    request req;
    std::string s = "POST / HTTP/1.1\r\nContent-Length: 5\r\n\r\nhello";
    std::vector<std::string> pieces;
    for ( char c : s ) pieces.push_back(std::string(1, c));
    auto ec = feed_pieces(p, req, pieces);
    expect_true(!ec, "ec success");
    expect_str(buf_to_string(req.body_container()), "hello", "body hello");
    std::cout << "  ok\n";
}

static void test_split_chunk_size_line()
{
    std::cout << "[test_split_chunk_size_line]\n";
    request_parser p;
    request req;
    auto ec = feed_pieces(p, req,
        {"POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n5\r",
         "\nhello\r\n0\r\n\r\n"});
    expect_true(!ec, "ec success");
    expect_str(buf_to_string(req.body_container()), "hello", "body hello");
    std::cout << "  ok\n";
}

static void test_split_chunk_data()
{
    std::cout << "[test_split_chunk_data]\n";
    request_parser p;
    request req;
    auto ec = feed_pieces(p, req,
        {"POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhe",
         "llo\r\n0\r\n\r\n"});
    expect_true(!ec, "ec success");
    expect_str(buf_to_string(req.body_container()), "hello", "body hello");
    std::cout << "  ok\n";
}

static void test_split_chunk_data_crlf()
{
    std::cout << "[test_split_chunk_data_crlf]\n";
    request_parser p;
    request req;
    auto ec = feed_pieces(p, req,
        {"POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r",
         "\n0\r\n\r\n"});
    expect_true(!ec, "ec success");
    expect_str(buf_to_string(req.body_container()), "hello", "body hello");
    std::cout << "  ok\n";
}

static void test_split_chunk0_trailer()
{
    std::cout << "[test_split_chunk0_trailer]\n";
    request_parser p;
    request req;
    auto ec = feed_pieces(p, req,
        {"POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n0\r\n",
         "X-T: v\r\n",
         "\r\n"});
    expect_true(!ec, "ec success");
    auto it = req.get_field("X-T");
    expect_true(it != req.field_end(), "trailer found");
    expect_str(it->second, "v", "trailer value");
    std::cout << "  ok\n";
}

static void test_split_trailer_empty_line()
{
    std::cout << "[test_split_trailer_empty_line]\n";
    request_parser p;
    request req;
    auto ec = feed_pieces(p, req,
        {"POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n0\r\nX-T: v\r",
         "\n\r\n"});
    expect_true(!ec, "ec success");
    auto it = req.get_field("X-T");
    expect_true(it != req.field_end(), "trailer found");
    std::cout << "  ok\n";
}

static void test_split_whole_message_byte_by_byte()
{
    std::cout << "[test_split_whole_message_byte_by_byte]\n";
    request_parser p;
    request req;
    std::string s =
        "POST /p HTTP/1.1\r\nHost: h\r\nTransfer-Encoding: chunked\r\n\r\n"
        "3\r\nabc\r\n2\r\nde\r\n0\r\nX-T: v\r\n\r\n";
    std::vector<std::string> pieces;
    for ( char c : s ) pieces.push_back(std::string(1, c));
    auto ec = feed_pieces(p, req, pieces);
    expect_true(!ec, "ec success");
    expect_true(req.get_method() == method::post, "method post");
    expect_str(req.get_target(), "/p", "target /p");
    expect_str(buf_to_string(req.body_container()), "abcde", "body abcde");
    auto it = req.get_field("X-T");
    expect_true(it != req.field_end(), "trailer found");
    std::cout << "  ok\n";
}

static void test_header_case_insensitive()
{
    std::cout << "[test_header_case_insensitive]\n";
    request_parser p;
    request req;
    feed_all(p, req, "POST / HTTP/1.1\r\ncontent-length: 5\r\n\r\nhello");
    expect_size(req.count("Content-Length"), 1, "count CL mixed");
    expect_size(req.count("content-length"), 1, "count cl lower");
    expect_size(req.count("CONTENT-LENGTH"), 1, "count CL upper");
    std::cout << "  ok\n";
}

static void test_duplicate_fields()
{
    std::cout << "[test_duplicate_fields]\n";
    request_parser p;
    request req;
    feed_all(p, req,
        "GET / HTTP/1.1\r\nSet-Cookie: a=1\r\nSet-Cookie: b=2\r\n\r\n");
    expect_size(req.count("set-cookie"), 2, "count set-cookie");
    std::vector<std::string> values;
    for ( auto it = req.field_begin(); it != req.field_end(); ++it ) {
        if ( it->first == "Set-Cookie" ) values.push_back(it->second);
    }
    expect_size(values.size(), 2, "two values");
    expect_str(values[0], "a=1", "first");
    expect_str(values[1], "b=2", "second");
    std::cout << "  ok\n";
}

static void test_duplicate_different_case()
{
    std::cout << "[test_duplicate_different_case]\n";
    request_parser p;
    request req;
    feed_all(p, req,
        "GET / HTTP/1.1\r\nSet-Cookie: a=1\r\nset-cookie: b=2\r\n\r\n");
    expect_size(req.count("set-cookie"), 2, "count mixed case");
    std::cout << "  ok\n";
}

static void test_te_case_variants()
{
    std::cout << "[test_te_case_variants]\n";
    const char* values[] = {
        "chunked", "Chunked", "CHUNKED", "gzip, chunked", "GZIP, CHUNKED"
    };
    for ( const char* v : values ) {
        request_parser p;
        request req;
        std::string s =
            std::string("POST / HTTP/1.1\r\nTransfer-Encoding: ") + v +
            "\r\n\r\n5\r\nhello\r\n0\r\n\r\n";
        auto ec = feed_all(p, req, s);
        expect_true(!ec, "ec success");
        expect_str(buf_to_string(req.body_container()), "hello", "body hello");
    }
    std::cout << "  ok\n";
}

static void test_te_beats_content_length()
{
    std::cout << "[test_te_beats_content_length]\n";
    request_parser p;
    request req;
    auto ec = feed_all(p, req,
        "POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n"
        "Content-Length: 999\r\n\r\n5\r\nhello\r\n0\r\n\r\n");
    expect_true(!ec, "ec success");
    expect_str(buf_to_string(req.body_container()), "hello", "body hello");
    std::cout << "  ok\n";
}

static void test_err_missing_space()
{
    std::cout << "[test_err_missing_space]\n";
    request_parser p;
    request req;
    auto ec = feed_all(p, req, "GET/\r\n\r\n");
    expect_ec(ec, lcy::bytra::http::error::invalid_header, "invalid_header");
    std::cout << "  ok\n";
}

static void test_err_unknown_method()
{
    std::cout << "[test_err_unknown_method]\n";
    request_parser p;
    request req;
    auto ec = feed_all(p, req, "FOO / HTTP/1.1\r\n\r\n");
    expect_ec(ec, lcy::bytra::http::error::invalid_header, "invalid_header");
    std::cout << "  ok\n";
}

static void test_err_invalid_version()
{
    std::cout << "[test_err_invalid_version]\n";
    const char* lines[] = {
        "GET / HTTP/2.0\r\n\r\n",
        "GET / HTTP/1\r\n\r\n",
        "GET / XYZ/1.1\r\n\r\n",
    };
    for ( const char* l : lines ) {
        request_parser p;
        request req;
        auto ec = feed_all(p, req, l);
        expect_ec(ec, lcy::bytra::http::error::invalid_header, l);
    }
    std::cout << "  ok\n";
}

static void test_err_status_not_number()
{
    std::cout << "[test_err_status_not_number]\n";
    response_parser p;
    response res;
    auto ec = feed_all(p, res, "HTTP/1.1 abc X\r\n\r\n");
    expect_ec(ec, lcy::bytra::http::error::invalid_header, "invalid_header");
    std::cout << "  ok\n";
}

static void test_err_status_out_of_range()
{
    std::cout << "[test_err_status_out_of_range]\n";
    const char* lines[] = {
        "HTTP/1.1 99 X\r\n\r\n",
        "HTTP/1.1 600 X\r\n\r\n",
    };
    for ( const char* l : lines ) {
        response_parser p;
        response res;
        auto ec = feed_all(p, res, l);
        expect_ec(ec, lcy::bytra::http::error::invalid_header, l);
    }
    std::cout << "  ok\n";
}

static void test_err_header_no_colon()
{
    std::cout << "[test_err_header_no_colon]\n";
    request_parser p;
    request req;
    auto ec = feed_all(p, req, "GET / HTTP/1.1\r\nHost a\r\n\r\n");
    expect_ec(ec, lcy::bytra::http::error::invalid_header, "invalid_header");
    std::cout << "  ok\n";
}

static void test_err_header_empty_name()
{
    std::cout << "[test_err_header_empty_name]\n";
    request_parser p;
    request req;
    auto ec = feed_all(p, req, "GET / HTTP/1.1\r\n: v\r\n\r\n");
    expect_ec(ec, lcy::bytra::http::error::invalid_header, "invalid_header");
    std::cout << "  ok\n";
}

static void test_err_content_length_not_number()
{
    std::cout << "[test_err_content_length_not_number]\n";
    request_parser p;
    request req;
    auto ec = feed_all(p, req,
        "POST / HTTP/1.1\r\nContent-Length: abc\r\n\r\n");
    expect_ec(ec, lcy::bytra::http::error::invalid_header, "invalid_header");
    std::cout << "  ok\n";
}

static void test_err_content_length_overflow()
{
    std::cout << "[test_err_content_length_overflow]\n";
    request_parser p;
    request req;
    auto ec = feed_all(p, req,
        "POST / HTTP/1.1\r\nContent-Length: 999999999999999999999999\r\n\r\n");
    expect_ec(ec, lcy::bytra::http::error::invalid_header, "invalid_header");
    std::cout << "  ok\n";
}

static void test_err_chunk_size_not_hex()
{
    std::cout << "[test_err_chunk_size_not_hex]\n";
    request_parser p;
    request req;
    auto ec = feed_all(p, req,
        "POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\nzz\r\n");
    expect_ec(ec, lcy::bytra::http::error::invalid_body, "invalid_body");
    std::cout << "  ok\n";
}

static void test_err_chunk_data_no_crlf()
{
    std::cout << "[test_err_chunk_data_no_crlf]\n";
    request_parser p;
    request req;
    auto ec = feed_all(p, req,
        "POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n"
        "5\r\nhelloXX0\r\n\r\n");
    expect_ec(ec, lcy::bytra::http::error::invalid_body, "invalid_body");
    std::cout << "  ok\n";
}

static void test_err_chunked_not_terminated()
{
    std::cout << "[test_err_chunked_not_terminated]\n";
    request_parser p;
    request req;
    auto ec = feed_all(p, req,
        "POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n");
    expect_ec(ec, lcy::bytra::http::error::wait_more, "wait_more");
    std::cout << "  ok\n";
}

static void test_err_trailer_no_colon()
{
    std::cout << "[test_err_trailer_no_colon]\n";
    request_parser p;
    request req;
    auto ec = feed_all(p, req,
        "POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n"
        "5\r\nhello\r\n0\r\nBadLine\r\n\r\n");
    expect_ec(ec, lcy::bytra::http::error::invalid_body, "invalid_body");
    std::cout << "  ok\n";
}

static void test_err_parse_header_in_body()
{
    std::cout << "[test_err_parse_header_in_body]\n";
    request_parser p;
    request req;
    lcy::system::error_code ec;
    const char* h = "POST / HTTP/1.1\r\nContent-Length: 5\r\n\r\n";
    p.parse(h, std::strlen(h), req, ec);
    expect_ec(ec, lcy::bytra::http::error::wait_more, "header done, wait body");
    p.parse_header("hello", 5, req, ec);
    expect_ec(ec, lcy::bytra::http::error::invalid_state, "invalid_state");
    std::cout << "  ok\n";
}

static void test_err_parse_body_in_header()
{
    std::cout << "[test_err_parse_body_in_header]\n";
    request_parser p;
    request req;
    lcy::system::error_code ec;
    p.parse_body("x", 1, req, ec);
    expect_ec(ec, lcy::bytra::http::error::invalid_state, "invalid_state");
    std::cout << "  ok\n";
}

static void test_parse_after_done()
{
    std::cout << "[test_parse_after_done]\n";
    request_parser p;
    request req;
    lcy::system::error_code ec;
    const char* s = "GET / HTTP/1.1\r\n\r\n";
    p.parse(s, std::strlen(s), req, ec);
    expect_true(!ec, "first parse success");
    ec = lcy::system::error_code();
    p.parse("X", 1, req, ec);
    expect_true(!ec, "parse after done success");
    std::cout << "  ok\n";
}

static void test_limit_header_too_large()
{
    std::cout << "[test_limit_header_too_large]\n";
    request_parser p;
    p.set_header_limit(100);
    request req;
    std::string garbage(200, 'x');
    auto ec = feed_all(p, req, garbage);
    expect_ec(ec, lcy::bytra::http::error::header_too_large, "header_too_large");
    std::cout << "  ok\n";
}

static void test_limit_header_within()
{
    std::cout << "[test_limit_header_within]\n";
    request_parser p;
    p.set_header_limit(100);
    request req;
    auto ec = feed_all(p, req, "GET / HTTP/1.1\r\nHost: a\r\n\r\n");
    expect_true(!ec, "within limit");
    std::cout << "  ok\n";
}

static void test_limit_body_fixed()
{
    std::cout << "[test_limit_body_fixed]\n";
    request_parser p;
    p.set_body_limit(10);
    request req;
    auto ec = feed_all(p, req,
        "POST / HTTP/1.1\r\nContent-Length: 20\r\n\r\n");
    expect_ec(ec, lcy::bytra::http::error::body_too_large, "body_too_large");
    std::cout << "  ok\n";
}

static void test_limit_body_chunked()
{
    std::cout << "[test_limit_body_chunked]\n";
    request_parser p;
    p.set_body_limit(10);
    request req;
    auto ec = feed_all(p, req,
        "POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n"
        "14\r\naaaaaaaaaaaaaaaaaaaa\r\n0\r\n\r\n");
    expect_ec(ec, lcy::bytra::http::error::body_too_large, "body_too_large");
    std::cout << "  ok\n";
}

static void test_limit_body_exact()
{
    std::cout << "[test_limit_body_exact]\n";
    request_parser p;
    p.set_body_limit(10);
    request req;
    auto ec = feed_all(p, req,
        "POST / HTTP/1.1\r\nContent-Length: 10\r\n\r\n0123456789");
    expect_true(!ec, "exact limit ok");
    expect_size(req.body_container().size(), 10, "body 10");
    std::cout << "  ok\n";
}

static void test_limit_header_not_triggered_by_body()
{
    std::cout << "[test_limit_header_not_triggered_by_body]\n";
    request_parser p;
    p.set_header_limit(100);
    request req;
    std::string body(1000, 'a');
    std::string s = "POST / HTTP/1.1\r\nContent-Length: 1000\r\n\r\n" + body;
    auto ec = feed_all(p, req, s);
    expect_true(!ec, "body doesn't trigger header limit");
    expect_size(req.body_container().size(), 1000, "body 1000");
    std::cout << "  ok\n";
}

static void test_reset_after_complete()
{
    std::cout << "[test_reset_after_complete]\n";
    request_parser p;
    request req;
    lcy::system::error_code ec;
    const char* s1 = "GET / HTTP/1.1\r\n\r\n";
    p.parse(s1, std::strlen(s1), req, ec);
    expect_true(!ec, "first success");
    p.reset();
    request req2;
    const char* s2 = "POST /x HTTP/1.1\r\n\r\n";
    p.parse(s2, std::strlen(s2), req2, ec);
    expect_true(!ec, "second success");
    expect_true(req2.get_method() == method::post, "method post");
    expect_str(req2.get_target(), "/x", "target /x");
    std::cout << "  ok\n";
}

static void test_reset_after_error()
{
    std::cout << "[test_reset_after_error]\n";
    request_parser p;
    request req;
    lcy::system::error_code ec;
    p.parse("BAD\r\n\r\n", 7, req, ec);
    expect_ec(ec, lcy::bytra::http::error::invalid_header, "err first");
    p.reset();
    request req2;
    const char* s = "GET / HTTP/1.1\r\n\r\n";
    p.parse(s, std::strlen(s), req2, ec);
    expect_true(!ec, "after reset success");
    std::cout << "  ok\n";
}

static void test_reset_clears_body()
{
    std::cout << "[test_reset_clears_body]\n";
    request_parser p;
    request req;
    lcy::system::error_code ec;
    const char* s1 = "POST / HTTP/1.1\r\nContent-Length: 2\r\n\r\nhi";
    p.parse(s1, std::strlen(s1), req, ec);
    expect_true(!ec, "first success");
    expect_size(req.body_container().size(), 2, "body 2");
    p.reset();
    request req2;
    const char* s2 = "GET / HTTP/1.1\r\n\r\n";
    p.parse(s2, std::strlen(s2), req2, ec);
    expect_true(!ec, "second success");
    expect_size(req2.body_container().size(), 0, "body 0");
    std::cout << "  ok\n";
}

static void test_reset_parsed_bytes()
{
    std::cout << "[test_reset_parsed_bytes]\n";
    request_parser p;
    request req;
    lcy::system::error_code ec;
   
    
    const char* s = "GET / HTTP/1.1\r\n\r\n";
    p.parse(s, std::strlen(s), req, ec);
    expect_true(!ec, "success");
    expect_size(p.get_parsed_bytes(), std::strlen(s), "parsed_bytes");
    p.reset();
    expect_size(p.get_parsed_bytes(), 0, "parsed_bytes after reset");
    std::cout << "  ok\n";
}

static void test_incremental_consume()
{
    std::cout << "[test_incremental_consume]\n";
    request_parser p;
    request req;
    flat_buffer buf;
    lcy::system::error_code ec;

    std::string s1 = "GET / HTT";
    std::string s2 = "P/1.1\r\nHost: a\r\n\r\n";

    auto v1 = buf.prepare(s1.size());
    std::memcpy(v1.data(), s1.data(), s1.size());
    buf.commit(s1.size());
    p.parse(static_cast<const char*>(buf.data()), buf.size(), req, ec);
    expect_ec(ec, lcy::bytra::http::error::wait_more, "first wait_more");
    std::size_t d1 = p.get_parsed_bytes();
    buf.consume(d1);
    expect_size(buf.size(), s1.size() - d1, "buffer remaining after first");

    auto v2 = buf.prepare(s2.size());
    std::memcpy(v2.data(), s2.data(), s2.size());
    buf.commit(s2.size());
    p.parse(static_cast<const char*>(buf.data()), buf.size(), req, ec);
    expect_true(!ec, "second success");
    std::size_t d2 = p.get_parsed_bytes() - d1;
    buf.consume(d2);
    expect_size(buf.size(), 0, "buffer empty");
    auto it = req.get_field("Host");
    expect_true(it != req.field_end(), "Host found");
    expect_str(it->second, "a", "Host value");
    std::cout << "  ok\n";
}

static void test_boundary_empty_input()
{
    std::cout << "[test_boundary_empty_input]\n";
    request_parser p;
    request req;
    lcy::system::error_code ec;
    p.parse(nullptr, 0, req, ec);
    expect_ec(ec, lcy::bytra::http::error::wait_more, "null 0 wait_more");

    ec = lcy::system::error_code();
    p.parse("", 0, req, ec);
    expect_ec(ec, lcy::bytra::http::error::wait_more, "empty wait_more");
    std::cout << "  ok\n";
}

static void test_boundary_one_byte()
{
    std::cout << "[test_boundary_one_byte]\n";
    request_parser p;
    request req;
    lcy::system::error_code ec;
    p.parse("G", 1, req, ec);
    expect_ec(ec, lcy::bytra::http::error::wait_more, "one byte wait_more");
    std::cout << "  ok\n";
}

static void test_boundary_empty_first_line()
{
    std::cout << "[test_boundary_empty_first_line]\n";
    request_parser p;
    request req;
    auto ec = feed_all(p, req, "\r\n\r\n");
    expect_ec(ec, lcy::bytra::http::error::invalid_header, "invalid_header");
    std::cout << "  ok\n";
}

static void test_boundary_header_value_with_colon()
{
    std::cout << "[test_boundary_header_value_with_colon]\n";
    request_parser p;
    request req;
    auto ec = feed_all(p, req, "GET / HTTP/1.1\r\nX: a:b:c\r\n\r\n");
    expect_true(!ec, "success");
    auto it = req.get_field("X");
    expect_true(it != req.field_end(), "X found");
    expect_str(it->second, "a:b:c", "X value");
    std::cout << "  ok\n";
}

static void test_boundary_header_value_ows()
{
    std::cout << "[test_boundary_header_value_ows]\n";
    request_parser p;
    request req;
    auto ec = feed_all(p, req, "GET / HTTP/1.1\r\nX:   v   \r\n\r\n");
    expect_true(!ec, "success");
    auto it = req.get_field("X");
    expect_true(it != req.field_end(), "X found");
    expect_str(it->second, "v", "X value trimmed");
    std::cout << "  ok\n";
}

static void test_independent_instances()
{
    std::cout << "[test_independent_instances]\n";
    request_parser p1, p2;
    request r1, r2;
    lcy::system::error_code ec1, ec2;
    const char* s1 = "GET /a HTTP/1.1\r\n\r\n";
    const char* s2 = "POST /b HTTP/1.1\r\nContent-Length: 2\r\n\r\nhi";
    p1.parse(s1, std::strlen(s1), r1, ec1);
    p2.parse(s2, std::strlen(s2), r2, ec2);
    expect_true(!ec1, "p1 success");
    expect_true(!ec2, "p2 success");
    expect_str(r1.get_target(), "/a", "r1 target");
    expect_str(r2.get_target(), "/b", "r2 target");
    expect_str(buf_to_string(r2.body_container()), "hi", "r2 body");
    std::cout << "  ok\n";
}

static void test_roundtrip_request()
{
    std::cout << "[test_roundtrip_request]\n";
    request req(method::post, "/p", version::http_1_1);
    req.set_field("Host", "a");
    std::string s = req.header_to_string();

    request_parser p;
    request parsed;
    auto ec = feed_all(p, parsed, s);
    expect_true(!ec, "success");
    expect_true(parsed.get_method() == method::post, "method post");
    expect_str(parsed.get_target(), "/p", "target /p");
    expect_true(parsed.get_version() == version::http_1_1, "version 1.1");
    auto it = parsed.get_field("Host");
    expect_true(it != parsed.field_end(), "Host found");
    expect_str(it->second, "a", "Host value");
    std::cout << "  ok\n";
}

static void test_roundtrip_response()
{
    std::cout << "[test_roundtrip_response]\n";
    response res(status::ok, version::http_1_1);
    res.set_field("X", "y");
    std::string s = res.header_to_string();

    response_parser p;
    response parsed;
    auto ec = feed_all(p, parsed, s);
    expect_true(!ec, "success");
    expect_true(parsed.get_status() == status::ok, "status ok");
    expect_str(parsed.get_reason(), "OK", "reason OK");
    auto it = parsed.get_field("X");
    expect_true(it != parsed.field_end(), "X found");
    expect_str(it->second, "y", "X value");
    std::cout << "  ok\n";
}

static void test_parse_header_then_body()
{
    std::cout << "[test_parse_header_then_body]\n";
    request_parser p;
    request req;
    lcy::system::error_code ec;

    const char* h = "POST /p HTTP/1.1\r\nContent-Length: 5\r\n\r\n";
    p.parse_header(h, std::strlen(h), req, ec);
    expect_true(!ec, "header success");
    expect_true(req.get_method() == method::post, "method post");
    expect_str(req.get_target(), "/p", "target /p");
    expect_size(req.count("Content-Length"), 1, "CL present");

    p.parse_body("hello", 5, req, ec);
    expect_true(!ec, "body success");
    expect_str(buf_to_string(req.body_container()), "hello", "body hello");

    std::cout << "  ok\n";
}

int main()
{
    test_request_minimal_get();
    test_request_with_headers();
    test_request_post_content_length();
    test_request_chunked();
    test_request_chunked_trailer();
    test_trailer_multiple_fields();
    test_trailer_same_name_as_header();
    test_trailer_name_split();
    test_trailer_done_after_empty_line();
    test_trailer_empty();
    test_response_chunked_trailer();
    test_trailer_value_with_colon_ows();
    test_request_chunk_extension();
    test_request_multiple_chunks();
    test_request_http10();
    test_response_minimal_200();
    test_response_no_reason();
    test_response_with_body();
    test_response_chunked();
    test_response_unknown_status_rejected();

    test_split_first_line();
    test_split_crlf();
    test_split_header_line();
    test_split_name_value();
    test_split_header_end();
    test_split_body_byte_by_byte();
    test_split_chunk_size_line();
    test_split_chunk_data();
    test_split_chunk_data_crlf();
    test_split_chunk0_trailer();
    test_split_trailer_empty_line();
    test_split_whole_message_byte_by_byte();

    test_header_case_insensitive();
    test_duplicate_fields();
    test_duplicate_different_case();
    test_te_case_variants();
    test_te_beats_content_length();

    test_err_missing_space();
    test_err_unknown_method();
    test_err_invalid_version();
    test_err_status_not_number();
    test_err_status_out_of_range();
    test_err_header_no_colon();
    test_err_header_empty_name();
    test_err_content_length_not_number();
    test_err_content_length_overflow();
    test_err_chunk_size_not_hex();
    test_err_chunk_data_no_crlf();
    test_err_chunked_not_terminated();
    test_err_trailer_no_colon();
    test_err_parse_header_in_body();
    test_err_parse_body_in_header();
    test_parse_after_done();

    test_limit_header_too_large();
    test_limit_header_within();
    test_limit_body_fixed();
    test_limit_body_chunked();
    test_limit_body_exact();
    test_limit_header_not_triggered_by_body();

    test_reset_after_complete();
    test_reset_after_error();
    test_reset_clears_body();
    test_reset_parsed_bytes();
    test_incremental_consume();

    test_boundary_empty_input();
    test_boundary_one_byte();
    test_boundary_empty_first_line();
    test_boundary_header_value_with_colon();
    test_boundary_header_value_ows();

    test_independent_instances();

    test_roundtrip_request();
    test_roundtrip_response();

    test_parse_header_then_body();

    std::cout << "\nall parser tests passed\n";
    return 0;
}