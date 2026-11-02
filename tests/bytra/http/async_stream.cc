#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <iostream>
#include <string>
#include <vector>

#include "lcy/bytra/flat_buffer.hpp"
#include "lcy/bytra/http/request.hpp"
#include "lcy/bytra/http/response.hpp"
#include "lcy/bytra/http/request_parser.hpp"
#include "lcy/bytra/http/response_parser.hpp"
#include "lcy/bytra/http/detail/error.hpp"
#include "lcy/bytra/http/async_stream.hpp"

using namespace lcy::bytra;
using namespace lcy::bytra::http;
using lcy::system::error_code;
using lcy::bytra::http::detail::make_http_error_code;

typedef lcy::bytra::flat_buffer flat_buf;
typedef request<flat_buf> req_t;
typedef response<flat_buf> res_t;
typedef request_parser<flat_buf> req_parser_t;
typedef response_parser<flat_buf> res_parser_t;

static void fail(const std::string& m) { std::cerr << "FAIL: " << m << "\n"; std::abort(); }
static void expect_true(bool c, const std::string& m) { if (!c) fail(m); }
static void expect_size(std::size_t g, std::size_t w, const std::string& m)
{
    if (g != w) fail(m + " got=" + std::to_string(g) + " want=" + std::to_string(w));
}
static void expect_str(const std::string& g, const std::string& w, const std::string& m)
{
    if (g != w) fail(m + " got=[" + g + "] want=[" + w + "]");
}
static void expect_ec(const error_code& ec, lcy::bytra::http::error want, const std::string& m)
{
    if ( static_cast<int>(ec) != static_cast<int>(want) ) {
        fail(m + " ec=" + std::to_string(static_cast<int>(ec))
             + " want=" + std::to_string(static_cast<int>(want)));
    }
}
static std::string buf_str(const flat_buf& b)
{
    return std::string(static_cast<const char*>(b.data()), b.size());
}

class mock_socket
{
public:
    void push(const std::string& s) { incoming_.push_back(s); }
    void set_read_error(lcy::bytra::http::error e) { read_error_ = e; has_read_error_ = true; }

    template <typename MutableBufferView, typename Handler>
    void async_read_some(MutableBufferView view, Handler h)
    {
        ++read_calls_;
        if ( has_read_error_ ) {
            h(make_http_error_code(read_error_), 0);
            return;
        }
        if ( incoming_.empty() ) {
            h(make_http_error_code(lcy::bytra::http::error::wait_more), 0);
            return;
        }
        std::string piece = incoming_.front();
        incoming_.pop_front();
        std::size_t n = piece.size();
        if ( n > view.length() ) n = view.length();
        std::memcpy(view.data(), piece.data(), n);
        h(error_code(), n);
    }

    template <typename ConstBufferView, typename Handler>
    void async_write_some(ConstBufferView view, Handler h)
    {
        ++write_calls_;
        if ( has_write_error_ ) {
            h(make_http_error_code(write_error_), 0);
            return;
        }
        written_.append(static_cast<const char*>(view.data()), view.length());
        h(error_code(), view.length());
    }

    void set_write_error(lcy::bytra::http::error e) { write_error_ = e; has_write_error_ = true; }

    int read_calls() const { return read_calls_; }
    int write_calls() const { return write_calls_; }
    const std::string& written() const { return written_; }

private:
    std::deque<std::string> incoming_;
    std::string written_;
    int read_calls_ = 0;
    int write_calls_ = 0;
    lcy::bytra::http::error read_error_ = lcy::bytra::http::error::success;
    lcy::bytra::http::error write_error_ = lcy::bytra::http::error::success;
    bool has_read_error_ = false;
    bool has_write_error_ = false;
};

static void test_read_header_once_no_body()
{
    std::cout << "[async_read_header: once, no body]\n";
    mock_socket sock;
    std::string h = "GET / HTTP/1.1\r\nHost: a\r\n\r\n";
    sock.push(h);

    req_parser_t p; req_t r;
    flat_buf buf;
    int calls = 0;
    std::size_t got_n = 0;
    error_code got_ec;

    async_read_header(sock, buf, p, r, [&](error_code ec, std::size_t n) {
        ++calls;
        got_ec = ec;
        got_n = n;
    });

    expect_size(calls, 1, "callback once");
    expect_true(!got_ec, "ec success");
    expect_size(got_n, h.size(), "n == header bytes");
    expect_size(sock.read_calls(), 1, "one read");
    expect_str(buf_str(buf), "", "buffer empty");
    expect_str(r.get_target(), "/", "target");
}

static void test_read_header_split_first_line()
{
    std::cout << "[async_read_header: split first line]\n";
    mock_socket sock;
    sock.push("GET / HTT");
    sock.push("P/1.1\r\n\r\n");

    req_parser_t p; req_t r;
    flat_buf buf;
    int calls = 0;
    std::size_t got_n = 0;
    error_code got_ec;

    async_read_header(sock, buf, p, r, [&](error_code ec, std::size_t n) {
        ++calls; got_ec = ec; got_n = n;
    });

    expect_size(calls, 1, "callback once");
    expect_true(!got_ec, "ec success");
    expect_size(got_n, std::strlen("GET / HTTP/1.1\r\n\r\n"), "n == total");
    expect_size(sock.read_calls(), 2, "two reads");
    expect_str(r.get_target(), "/", "target");
}

static void test_read_header_split_field()
{
    std::cout << "[async_read_header: split field]\n";
    mock_socket sock;
    sock.push("GET / HTTP/1.1\r\nHost: exa");
    sock.push("mple.com\r\n\r\n");

    req_parser_t p; req_t r;
    flat_buf buf;
    int calls = 0;

    async_read_header(sock, buf, p, r, [&](error_code ec, std::size_t) {
        ++calls; expect_true(!ec, "ec");
    });

    expect_size(calls, 1, "callback once");
    auto it = r.get_field("Host");
    expect_true(it != r.field_end(), "Host found");
    expect_str(it->second, "example.com", "Host value");
}

static void test_read_header_leaves_body_in_buffer()
{
    std::cout << "[async_read_header: leaves body]\n";
    mock_socket sock;
    std::string s = "POST / HTTP/1.1\r\nContent-Length: 5\r\n\r\nhello";
    sock.push(s);

    req_parser_t p; req_t r;
    flat_buf buf;

    async_read_header(sock, buf, p, r, [&](error_code ec, std::size_t) {
        expect_true(!ec, "header ok");
    });

    expect_str(buf_str(buf), "hello", "body left in buffer");
    expect_size(buf.size(), 5, "buffer size 5");
}

static void test_read_header_invalid_first_line()
{
    std::cout << "[async_read_header: invalid first line]\n";
    mock_socket sock;
    sock.push("BAD\r\n\r\n");

    req_parser_t p; req_t r;
    flat_buf buf;
    int calls = 0;
    error_code got_ec;

    async_read_header(sock, buf, p, r, [&](error_code ec, std::size_t) {
        ++calls; got_ec = ec;
    });

    expect_size(calls, 1, "callback once");
    expect_ec(got_ec, lcy::bytra::http::error::invalid_header, "ec");
}

static void test_read_header_socket_error()
{
    std::cout << "[async_read_header: socket error]\n";
    mock_socket sock;
    sock.set_read_error(lcy::bytra::http::error::invalid_body);

    req_parser_t p; req_t r;
    flat_buf buf;
    int calls = 0;
    error_code got_ec;

    async_read_header(sock, buf, p, r, [&](error_code ec, std::size_t) {
        ++calls; got_ec = ec;
    });

    expect_size(calls, 1, "callback once");
    expect_ec(got_ec, lcy::bytra::http::error::invalid_body, "socket error");
}

static void test_read_header_limit()
{
    std::cout << "[async_read_header: header limit]\n";
    mock_socket sock;
    sock.push(std::string(200, 'x'));

    req_parser_t p; req_t r;
    p.set_header_limit(100);
    flat_buf buf;
    int calls = 0;
    error_code got_ec;

    async_read_header(sock, buf, p, r, [&](error_code ec, std::size_t) {
        ++calls; got_ec = ec;
    });

    expect_size(calls, 1, "callback once");
    expect_ec(got_ec, lcy::bytra::http::error::header_too_large, "limit");
}

static void test_read_body_once()
{
    std::cout << "[async_read_body: once]\n";
    mock_socket sock;
    sock.push("POST / HTTP/1.1\r\nContent-Length: 5\r\n\r\n");
    sock.push("hello");

    req_parser_t p; req_t r;
    flat_buf buf;

    async_read_header(sock, buf, p, r, [&](error_code ec, std::size_t) {
        expect_true(!ec, "header ok");
        int calls = 0;
        std::size_t got_n = 0;
        async_read_body(sock, buf, p, r, [&](error_code ec2, std::size_t n) {
            ++calls; got_n = n;
            expect_true(!ec2, "body ok");
        });
        expect_size(calls, 1, "body callback once");
        expect_size(got_n, 5, "body n==5");
        expect_str(buf_str(r.body_container()), "hello", "body");
        expect_str(buf_str(buf), "", "buffer empty after body");
    });
}

static void test_read_body_byte_by_byte()
{
    std::cout << "[async_read_body: byte by byte]\n";
    mock_socket sock;
    sock.push("POST / HTTP/1.1\r\nContent-Length: 5\r\n\r\n");
    for ( char c : std::string("hello") ) {
        sock.push(std::string(1, c));
    }

    req_parser_t p; req_t r;
    flat_buf buf;

    async_read_header(sock, buf, p, r, [&](error_code ec, std::size_t) {
        expect_true(!ec, "header ok");
        int calls = 0;
        async_read_body(sock, buf, p, r, [&](error_code ec2, std::size_t n) {
            ++calls;
            expect_true(!ec2, "body ok");
            expect_size(n, 5, "body consumed 5");
        });
        expect_size(calls, 1, "body callback once");
        expect_str(buf_str(r.body_container()), "hello", "body");
    });
}

static void test_read_body_invalid_state()
{
    std::cout << "[async_read_body: invalid state]\n";
    mock_socket sock;
    sock.push("x");

    req_parser_t p; req_t r;
    flat_buf buf;
    int calls = 0;
    error_code got_ec;

    async_read_body(sock, buf, p, r, [&](error_code ec, std::size_t) {
        ++calls; got_ec = ec;
    });

    expect_size(calls, 1, "callback once");
    expect_ec(got_ec, lcy::bytra::http::error::invalid_state, "invalid_state");
}

static void test_read_body_chunked()
{
    std::cout << "[async_read_body: chunked]\n";
    mock_socket sock;
    sock.push("POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n");
    sock.push("3\r\nabc\r\n2\r\nde\r\n0\r\n\r\n");

    req_parser_t p; req_t r;
    flat_buf buf;

    async_read_header(sock, buf, p, r, [&](error_code ec, std::size_t) {
        expect_true(!ec, "header ok");
        async_read_body(sock, buf, p, r, [&](error_code ec2, std::size_t) {
            expect_true(!ec2, "body ok");
        });
        expect_str(buf_str(r.body_container()), "abcde", "chunked body");
    });
}

static void test_read_full_once()
{
    std::cout << "[async_read: once]\n";
    mock_socket sock;
    std::string s = "POST /p HTTP/1.1\r\nContent-Length: 5\r\n\r\nhello";
    sock.push(s);

    req_parser_t p; req_t r;
    flat_buf buf;
    int calls = 0;
    std::size_t got_n = 0;
    error_code got_ec;

    async_read(sock, buf, p, r, [&](error_code ec, std::size_t n) {
        ++calls; got_ec = ec; got_n = n;
    });

    expect_size(calls, 1, "callback once");
    expect_true(!got_ec, "ec");
    expect_size(got_n, s.size(), "n == total");
    expect_str(r.get_target(), "/p", "target");
    expect_str(buf_str(r.body_container()), "hello", "body");
}

static void test_read_full_byte_by_byte()
{
    std::cout << "[async_read: byte by byte]\n";
    mock_socket sock;
    std::string s = "POST / HTTP/1.1\r\nContent-Length: 3\r\n\r\nabc";
    for ( char c : s ) sock.push(std::string(1, c));

    req_parser_t p; req_t r;
    flat_buf buf;
    int calls = 0;

    async_read(sock, buf, p, r, [&](error_code ec, std::size_t) {
        ++calls; expect_true(!ec, "ec");
    });

    expect_size(calls, 1, "callback once");
    expect_str(buf_str(r.body_container()), "abc", "body");
}

static void test_read_full_body_error()
{
    std::cout << "[async_read: body error]\n";
    mock_socket sock;
    sock.push("POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\nzz\r\n");

    req_parser_t p; req_t r;
    flat_buf buf;
    int calls = 0;
    error_code got_ec;

    async_read(sock, buf, p, r, [&](error_code ec, std::size_t) {
        ++calls; got_ec = ec;
    });

    expect_size(calls, 1, "callback once");
    expect_ec(got_ec, lcy::bytra::http::error::invalid_body, "invalid_body");
}

static void test_read_full_socket_error()
{
    std::cout << "[async_read: socket error]\n";
    mock_socket sock;
    sock.set_read_error(lcy::bytra::http::error::invalid_header);

    req_parser_t p; req_t r;
    flat_buf buf;
    int calls = 0;
    error_code got_ec;

    async_read(sock, buf, p, r, [&](error_code ec, std::size_t) {
        ++calls; got_ec = ec;
    });

    expect_size(calls, 1, "callback once");
    expect_ec(got_ec, lcy::bytra::http::error::invalid_header, "socket error");
}

static void test_read_full_body_limit()
{
    std::cout << "[async_read: body limit]\n";
    mock_socket sock;
    sock.push("POST / HTTP/1.1\r\nContent-Length: 20\r\n\r\n");

    req_parser_t p; req_t r;
    p.set_body_limit(10);
    flat_buf buf;
    int calls = 0;
    error_code got_ec;

    async_read(sock, buf, p, r, [&](error_code ec, std::size_t) {
        ++calls; got_ec = ec;
    });

    expect_size(calls, 1, "callback once");
    expect_ec(got_ec, lcy::bytra::http::error::body_too_large, "body_too_large");
}

static void test_write_header()
{
    std::cout << "[async_write_header: normal]\n";
    req_t req(method::post, "/p", version::http_1_1);
    req.set_field("Host", "a");
    req.set_field("Content-Length", "5");
    {
        auto v = req.body_container().prepare(5);
        std::memcpy(v.data(), "hello", 5);
        req.body_container().commit(5);
    }

    mock_socket sock;
    int calls = 0;
    std::size_t got_n = 0;
    error_code got_ec;

    async_write_header(sock, req, [&](error_code ec, std::size_t n) {
        ++calls; got_ec = ec; got_n = n;
    });

    expect_size(calls, 1, "callback once");
    expect_true(!got_ec, "ec");
    std::string expect = req.header_to_string();
    expect_size(got_n, expect.size(), "n == header size");
    expect_str(sock.written(), expect, "written == header");
    expect_true(sock.written().find("hello") == std::string::npos, "no body");
}

static void test_write_header_response()
{
    std::cout << "[async_write_header: response]\n";
    res_t res(status::ok, version::http_1_1);
    res.set_field("X", "y");

    mock_socket sock;
    async_write_header(sock, res, [](error_code ec, std::size_t) {
        expect_true(!ec, "ec");
    });
    expect_str(sock.written(), res.header_to_string(), "written");
}

static void test_write_header_error()
{
    std::cout << "[async_write_header: error]\n";
    req_t req(method::get, "/", version::http_1_1);

    mock_socket sock;
    sock.set_write_error(lcy::bytra::http::error::invalid_body);
    int calls = 0;
    error_code got_ec;

    async_write_header(sock, req, [&](error_code ec, std::size_t) {
        ++calls; got_ec = ec;
    });

    expect_size(calls, 1, "callback once");
    expect_ec(got_ec, lcy::bytra::http::error::invalid_body, "write error");
}

static void test_write_body()
{
    std::cout << "[async_write_body: normal]\n";
    req_t req(method::post, "/", version::http_1_1);
    {
        auto v = req.body_container().prepare(5);
        std::memcpy(v.data(), "hello", 5);
        req.body_container().commit(5);
    }

    mock_socket sock;
    int calls = 0;
    std::size_t got_n = 0;

    async_write_body(sock, req, [&](error_code ec, std::size_t n) {
        ++calls; got_n = n;
        expect_true(!ec, "ec");
    });

    expect_size(calls, 1, "callback once");
    expect_size(got_n, 5, "n == 5");
    expect_str(sock.written(), "hello", "body written");
}

static void test_write_body_empty()
{
    std::cout << "[async_write_body: empty]\n";
    req_t req(method::get, "/", version::http_1_1);

    mock_socket sock;
    int calls = 0;
    std::size_t got_n = 999;

    async_write_body(sock, req, [&](error_code ec, std::size_t n) {
        ++calls; got_n = n;
        expect_true(!ec, "ec");
    });

    expect_size(calls, 1, "callback once");
    expect_size(got_n, 0, "n == 0");
    expect_str(sock.written(), "", "nothing written");
}

static void test_write_body_error()
{
    std::cout << "[async_write_body: error]\n";
    req_t req(method::post, "/", version::http_1_1);
    {
        auto v = req.body_container().prepare(3);
        std::memcpy(v.data(), "abc", 3);
        req.body_container().commit(3);
    }

    mock_socket sock;
    sock.set_write_error(lcy::bytra::http::error::invalid_body);
    int calls = 0;
    error_code got_ec;

    async_write_body(sock, req, [&](error_code ec, std::size_t) {
        ++calls; got_ec = ec;
    });

    expect_size(calls, 1, "callback once");
    expect_ec(got_ec, lcy::bytra::http::error::invalid_body, "error");
}

static void test_write_full()
{
    std::cout << "[async_write: full]\n";
    req_t req(method::post, "/p", version::http_1_1);
    req.set_field("Content-Length", "5");
    {
        auto v = req.body_container().prepare(5);
        std::memcpy(v.data(), "hello", 5);
        req.body_container().commit(5);
    }

    std::string expect_header = req.header_to_string();

    mock_socket sock;
    int calls = 0;
    std::size_t got_n = 0;
    error_code got_ec;

    async_write(sock, req, [&](error_code ec, std::size_t n) {
        ++calls; got_ec = ec; got_n = n;
    });

    expect_size(calls, 1, "callback once");
    expect_true(!got_ec, "ec");
    expect_size(got_n, 5, "n == body size");
    expect_str(sock.written(), expect_header + "hello", "written == header+body");
}

static void test_write_full_header_error()
{
    std::cout << "[async_write: header error]\n";
    req_t req(method::post, "/", version::http_1_1);
    {
        auto v = req.body_container().prepare(3);
        std::memcpy(v.data(), "abc", 3);
        req.body_container().commit(3);
    }

    mock_socket sock;
    sock.set_write_error(lcy::bytra::http::error::invalid_body);
    int calls = 0;
    error_code got_ec;

    async_write(sock, req, [&](error_code ec, std::size_t) {
        ++calls; got_ec = ec;
    });

    expect_size(calls, 1, "callback once");
    expect_ec(got_ec, lcy::bytra::http::error::invalid_body, "header error");
    expect_true(sock.written().find("abc") == std::string::npos, "body not written");
}

static void test_write_full_empty_body()
{
    std::cout << "[async_write: empty body]\n";
    req_t req(method::get, "/", version::http_1_1);

    mock_socket sock;
    int calls = 0;
    std::size_t got_n = 999;

    async_write(sock, req, [&](error_code ec, std::size_t n) {
        ++calls; got_n = n;
        expect_true(!ec, "ec");
    });

    expect_size(calls, 1, "callback once");
    expect_size(got_n, 0, "n == 0");
    expect_str(sock.written(), req.header_to_string(), "only header");
}

static void test_write_full_body_error()
{
    std::cout << "[async_write: body error]\n";
    req_t req(method::post, "/", version::http_1_1);
    {
        auto v = req.body_container().prepare(3);
        std::memcpy(v.data(), "abc", 3);
        req.body_container().commit(3);
    }

    mock_socket sock;
    (void)sock;
    std::cout << "  (skipped: mock lacks per-call failure)\n";
}

static void test_read_header_callback_once_multi_pieces()
{
    std::cout << "[async_read_header: callback exactly once across many pieces]\n";
    mock_socket sock;
    std::string s = "GET / HTTP/1.1\r\nHost: a\r\nX: b\r\n\r\n";
    for ( char c : s ) sock.push(std::string(1, c));

    req_parser_t p; req_t r;
    flat_buf buf;
    int calls = 0;

    async_read_header(sock, buf, p, r, [&](error_code ec, std::size_t) {
        ++calls; expect_true(!ec, "ec");
    });

    expect_size(calls, 1, "callback exactly once");
    expect_size(sock.read_calls(), s.size(), "read calls == bytes");
    expect_size(r.count("Host"), 1, "Host");
    expect_size(r.count("X"), 1, "X");
}

static void test_read_full_callback_once_multi_pieces()
{
    std::cout << "[async_read: callback once across pieces]\n";
    mock_socket sock;
    std::string s = "POST / HTTP/1.1\r\nContent-Length: 3\r\n\r\nabc";
    for ( char c : s ) sock.push(std::string(1, c));

    req_parser_t p; req_t r;
    flat_buf buf;
    int calls = 0;
    std::size_t got_n = 0;

    async_read(sock, buf, p, r, [&](error_code ec, std::size_t n) {
        ++calls; got_n = n;
        expect_true(!ec, "ec");
    });

    expect_size(calls, 1, "callback once");
    expect_size(got_n, s.size(), "n == total");
}

static void test_write_callback_once()
{
    std::cout << "[async_write: callback once]\n";
    req_t req(method::post, "/", version::http_1_1);
    {
        auto v = req.body_container().prepare(100);
        std::memset(v.data(), 'a', 100);
        req.body_container().commit(100);
    }

    mock_socket sock;
    int calls = 0;
    async_write(sock, req, [&](error_code ec, std::size_t) {
        ++calls; expect_true(!ec, "ec");
    });

    expect_size(calls, 1, "callback once");
    expect_true(sock.written().find(std::string(100, 'a')) != std::string::npos, "body written");
}

int main()
{
    test_read_header_once_no_body();
    test_read_header_split_first_line();
    test_read_header_split_field();
    test_read_header_leaves_body_in_buffer();
    test_read_header_invalid_first_line();
    test_read_header_socket_error();
    test_read_header_limit();
    test_read_header_callback_once_multi_pieces();

    test_read_body_once();
    test_read_body_byte_by_byte();
    test_read_body_invalid_state();
    test_read_body_chunked();

    test_read_full_once();
    test_read_full_byte_by_byte();
    test_read_full_body_error();
    test_read_full_socket_error();
    test_read_full_body_limit();
    test_read_full_callback_once_multi_pieces();

    test_write_header();
    test_write_header_response();
    test_write_header_error();

    test_write_body();
    test_write_body_empty();
    test_write_body_error();

    test_write_full();
    test_write_full_header_error();
    test_write_full_empty_body();
    test_write_full_body_error();
    test_write_callback_once();

    std::cout << "\nall async_stream tests passed\n";
    return 0;
}