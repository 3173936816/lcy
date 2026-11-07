#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "lcy/asio/io_context.hpp"
#include "lcy/asio/ip/tcp.hpp"
#include "lcy/asio/buffer_view.hpp"
#include "lcy/asio/ssl/context.hpp"
#include "lcy/asio/ssl/stream_socket.hpp"

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

static void expect_ec_ok(const lcy::system::error_code& ec, const char* msg)
{
    if ( ec ) {
        std::cerr << "FAIL: " << msg << "\n  ec: " << ec.message() << "\n";
        std::abort();
    }
}

static void expect_ec_err(const lcy::system::error_code& ec, const char* msg)
{
    if ( !ec ) {
        std::cerr << "FAIL: " << msg << " (expected error)\n";
        std::abort();
    }
}

static void connect_tcp(lcy::asio::ssl::stream_socket& ssl_socket,
    const std::string& host, const std::string& port,
    lcy::system::error_code& ec)
{
    lcy::asio::ip::tcp::resolver resolver;
    std::vector<lcy::asio::ip::tcp::endpoint> endpoints;
    resolver.resolve(host, port, lcy::asio::ip::tcp::v4(), endpoints, ec);
    expect_ec_ok(ec, "resolve");

    expect_true(!endpoints.empty(), "endpoints not empty");

    ssl_socket.get_next_layer().open(lcy::asio::ip::tcp::v4(), ec);
    expect_ec_ok(ec, "open");

    ssl_socket.get_next_layer().connect(endpoints[0], ec);
    expect_ec_ok(ec, "connect");
}

static void make_connected_ssl_socket(
    lcy::asio::ssl::context& ssl_ctx,
    lcy::asio::ssl::stream_socket& ssl_socket)
{
    lcy::system::error_code ec;
    ssl_ctx.set_default_verify_paths(ec);
    expect_ec_ok(ec, "set_default_verify_paths");
    ssl_ctx.set_verify_mode(lcy::asio::ssl::verify_peer);

    ssl_socket.set_hostname("www.baidu.com", ec);
    expect_ec_ok(ec, "set_hostname");
    ssl_socket.set_verify_hostname("www.baidu.com", ec);
    expect_ec_ok(ec, "set_verify_hostname");

    connect_tcp(ssl_socket, "www.baidu.com", "443", ec);
}

static void test_set_hostname_and_verify_hostname()
{
    std::cout << "[test_set_hostname_and_verify_hostname]\n";
    lcy::asio::io_context ioc;
    lcy::asio::ssl::context ssl_ctx(lcy::asio::ssl::context::tls_client);
    lcy::asio::ssl::stream_socket ssl_socket(ioc, ssl_ctx);

    lcy::system::error_code ec;
    ssl_socket.set_hostname("www.baidu.com", ec);
    expect_ec_ok(ec, "set_hostname");

    ssl_socket.set_verify_hostname("www.baidu.com", ec);
    expect_ec_ok(ec, "set_verify_hostname");

    ssl_socket.set_hostname("www.baidu.com", ec);
    expect_ec_ok(ec, "set_hostname again");

    ssl_socket.set_hostname("", ec);
    (void)ec;

    std::cout << "  ok\n";
}

static void test_context_config()
{
    std::cout << "[test_context_config]\n";
    lcy::asio::ssl::context ssl_ctx(lcy::asio::ssl::context::tls_client);

    lcy::system::error_code ec;
    ssl_ctx.set_default_verify_paths(ec);
    expect_ec_ok(ec, "set_default_verify_paths");

    ssl_ctx.set_verify_mode(lcy::asio::ssl::verify_peer);
    ssl_ctx.set_verify_mode(lcy::asio::ssl::verify_none);
    ssl_ctx.set_verify_mode(lcy::asio::ssl::verify_peer);

    ssl_ctx.set_options(lcy::asio::ssl::context::no_sslv3);
    ssl_ctx.set_options(lcy::asio::ssl::context::no_tlsv1);

    ssl_ctx.load_verify_file("/nonexistent/ca.pem", ec);
    expect_ec_err(ec, "load_verify_file nonexistent");

    ssl_ctx.use_certificate_file("/nonexistent/cert.pem",
        lcy::asio::ssl::context::pem, ec);
    expect_ec_err(ec, "use_certificate_file nonexistent");

    ssl_ctx.use_private_key_file("/nonexistent/key.pem",
        lcy::asio::ssl::context::pem, ec);
    expect_ec_err(ec, "use_private_key_file nonexistent");

    std::cout << "  ok\n";
}

static void test_handshake_ok()
{
    std::cout << "[test_handshake_ok]\n";
    lcy::asio::io_context ioc;
    lcy::asio::ssl::context ssl_ctx(lcy::asio::ssl::context::tls_client);
    lcy::asio::ssl::stream_socket ssl_socket(ioc, ssl_ctx);
    lcy::system::error_code ec;

    make_connected_ssl_socket(ssl_ctx, ssl_socket);

    ssl_socket.client_handshake(ec);
    expect_ec_ok(ec, "client_handshake");

    ssl_socket.close(ec);
    expect_ec_ok(ec, "close");
    std::cout << "  ok\n";
}

static void test_write_read_some()
{
    std::cout << "[test_write_read_some]\n";
    lcy::asio::io_context ioc;
    lcy::asio::ssl::context ssl_ctx(lcy::asio::ssl::context::tls_client);
    lcy::asio::ssl::stream_socket ssl_socket(ioc, ssl_ctx);
    lcy::system::error_code ec;

    make_connected_ssl_socket(ssl_ctx, ssl_socket);
    ssl_socket.client_handshake(ec);
    expect_ec_ok(ec, "client_handshake");

    std::string req = "GET / HTTP/1.1\r\nHost: www.baidu.com\r\nConnection: close\r\n\r\n";
    std::size_t n = ssl_socket.write_some(
        lcy::asio::buffer_view(&req[0], req.length()), ec);
    expect_ec_ok(ec, "write_some");
    expect_size(n, req.length(), "write_some full");

    std::string resp;
    resp.resize(4096);
    std::size_t rn = ssl_socket.read_some(
        lcy::asio::buffer_view(&resp[0], resp.length()), ec);
    expect_ec_ok(ec, "read_some");
    expect_true(rn > 0, "read_some > 0");

    ssl_socket.close(ec);
    expect_ec_ok(ec, "close");
    std::cout << "  ok\n";
}

static void test_write_read_throw_versions()
{
    std::cout << "[test_write_read_throw_versions]\n";
    lcy::asio::io_context ioc;
    lcy::asio::ssl::context ssl_ctx(lcy::asio::ssl::context::tls_client);
    lcy::asio::ssl::stream_socket ssl_socket(ioc, ssl_ctx);
    lcy::system::error_code ec;

    make_connected_ssl_socket(ssl_ctx, ssl_socket);
    ssl_socket.client_handshake();

    std::string req = "GET / HTTP/1.1\r\nHost: www.baidu.com\r\nConnection: close\r\n\r\n";
    std::size_t n = ssl_socket.write_some(
        lcy::asio::buffer_view(&req[0], req.length()));
    expect_size(n, req.length(), "write_some throw version");

    std::string resp;
    resp.resize(4096);
    std::size_t rn = ssl_socket.read_some(
        lcy::asio::buffer_view(&resp[0], resp.length()));
    expect_true(rn > 0, "read_some throw version > 0");

    ssl_socket.close();
    std::cout << "  ok\n";
}

static void test_short_read_write()
{
    std::cout << "[test_short_read_write]\n";
    lcy::asio::io_context ioc;
    lcy::asio::ssl::context ssl_ctx(lcy::asio::ssl::context::tls_client);
    lcy::asio::ssl::stream_socket ssl_socket(ioc, ssl_ctx);
    lcy::system::error_code ec;

    make_connected_ssl_socket(ssl_ctx, ssl_socket);
    ssl_socket.client_handshake(ec);
    expect_ec_ok(ec, "client_handshake");

    std::string req = "GET / HTTP/1.1\r\nHost: www.baidu.com\r\nConnection: close\r\n\r\n";
    std::size_t total = 0;
    while ( total < req.length() ) {
        std::size_t n = ssl_socket.write_some(
            lcy::asio::buffer_view(&req[total], req.length() - total), ec);
        expect_ec_ok(ec, "short write_some");
        expect_true(n > 0, "short write_some > 0");
        total += n;
    }
    expect_size(total, req.length(), "short write total");

    std::string resp;
    resp.resize(4096);
    std::size_t rn = ssl_socket.read_some(
        lcy::asio::buffer_view(&resp[0], 1), ec);
    expect_ec_ok(ec, "short read_some");
    expect_true(rn == 1 || rn > 0, "short read_some > 0");

    ssl_socket.close(ec);
    expect_ec_ok(ec, "close");
    std::cout << "  ok\n";
}

static void test_handshake_twice()
{
    std::cout << "[test_handshake_twice]\n";
    lcy::asio::io_context ioc;
    lcy::asio::ssl::context ssl_ctx(lcy::asio::ssl::context::tls_client);
    lcy::asio::ssl::stream_socket ssl_socket(ioc, ssl_ctx);
    lcy::system::error_code ec;

    make_connected_ssl_socket(ssl_ctx, ssl_socket);

    ssl_socket.client_handshake(ec);
    expect_ec_ok(ec, "first handshake");
    ssl_socket.client_handshake(ec);
    expect_ec_ok(ec, "second handshake");

    ssl_socket.close(ec);
    expect_ec_ok(ec, "close");
    std::cout << "  ok\n";
}

static void test_close_twice()
{
    std::cout << "[test_close_twice]\n";
    lcy::asio::io_context ioc;
    lcy::asio::ssl::context ssl_ctx(lcy::asio::ssl::context::tls_client);
    lcy::asio::ssl::stream_socket ssl_socket(ioc, ssl_ctx);
    lcy::system::error_code ec;

    make_connected_ssl_socket(ssl_ctx, ssl_socket);
    ssl_socket.client_handshake(ec);
    expect_ec_ok(ec, "client_handshake");

    ssl_socket.close(ec);
    expect_ec_ok(ec, "first close");
    ssl_socket.close(ec);
    expect_ec_err(ec, "second close (idempotent)");
    std::cout << "  ok\n";
}

static void test_write_after_close()
{
    std::cout << "[test_write_after_close]\n";
    lcy::asio::io_context ioc;
    lcy::asio::ssl::context ssl_ctx(lcy::asio::ssl::context::tls_client);
    lcy::asio::ssl::stream_socket ssl_socket(ioc, ssl_ctx);
    lcy::system::error_code ec;

    make_connected_ssl_socket(ssl_ctx, ssl_socket);
    ssl_socket.client_handshake(ec);
    expect_ec_ok(ec, "client_handshake");

    ssl_socket.close(ec);
    expect_ec_ok(ec, "close");

    std::string req = "GET / HTTP/1.1\r\n\r\n";
    ssl_socket.write_some(lcy::asio::buffer_view(&req[0], req.length()), ec);
    expect_ec_err(ec, "write after close");
    std::cout << "  ok\n";
}

static void test_read_after_close()
{
    std::cout << "[test_read_after_close]\n";
    lcy::asio::io_context ioc;
    lcy::asio::ssl::context ssl_ctx(lcy::asio::ssl::context::tls_client);
    lcy::asio::ssl::stream_socket ssl_socket(ioc, ssl_ctx);
    lcy::system::error_code ec;

    make_connected_ssl_socket(ssl_ctx, ssl_socket);
    ssl_socket.client_handshake(ec);
    expect_ec_ok(ec, "client_handshake");

    ssl_socket.close(ec);
    expect_ec_ok(ec, "close");

    std::string resp;
    resp.resize(4096);
    ssl_socket.read_some(lcy::asio::buffer_view(&resp[0], resp.length()), ec);
    expect_ec_err(ec, "read after close");
    std::cout << "  ok\n";
}

static void test_handshake_without_connect()
{
    std::cout << "[test_handshake_without_connect]\n";
    lcy::asio::io_context ioc;
    lcy::asio::ssl::context ssl_ctx(lcy::asio::ssl::context::tls_client);
    lcy::asio::ssl::stream_socket ssl_socket(ioc, ssl_ctx);
    lcy::system::error_code ec;

    ssl_socket.set_hostname("www.baidu.com", ec);
    expect_ec_ok(ec, "set_hostname");

    ssl_socket.client_handshake(ec);
    expect_ec_err(ec, "handshake without connect");
    std::cout << "  ok\n";
}

static void test_double_handshake_after_close()
{
    std::cout << "[test_double_handshake_after_close]\n";
    lcy::asio::io_context ioc;
    lcy::asio::ssl::context ssl_ctx(lcy::asio::ssl::context::tls_client);
    lcy::asio::ssl::stream_socket ssl_socket(ioc, ssl_ctx);
    lcy::system::error_code ec;

    make_connected_ssl_socket(ssl_ctx, ssl_socket);
    ssl_socket.client_handshake(ec);
    expect_ec_ok(ec, "client_handshake");

    ssl_socket.close(ec);
    expect_ec_ok(ec, "close");

    ssl_socket.client_handshake(ec);
    expect_ec_err(ec, "handshake after close");
    std::cout << "  ok\n";
}

static void test_async_client_handshake()
{
    std::cout << "[test_async_client_handshake]\n";
    lcy::asio::io_context ioc;
    lcy::asio::ssl::context ssl_ctx(lcy::asio::ssl::context::tls_client);
    lcy::asio::ssl::stream_socket ssl_socket(ioc, ssl_ctx);
    lcy::system::error_code ec;

    make_connected_ssl_socket(ssl_ctx, ssl_socket);

    bool called = false;
    lcy::system::error_code result_ec;
    ssl_socket.async_client_handshake(
        [&called, &result_ec](lcy::system::error_code ec) {
            called = true;
            result_ec = ec;
        });

    ioc.run();

    expect_true(called, "async handshake called");
    expect_ec_ok(result_ec, "async handshake result");

    ssl_socket.close(ec);
    expect_ec_ok(ec, "close");
    std::cout << "  ok\n";
}

static void test_async_read_write_some()
{
    std::cout << "[test_async_read_write_some]\n";
    lcy::asio::io_context ioc;
    lcy::asio::ssl::context ssl_ctx(lcy::asio::ssl::context::tls_client);
    lcy::asio::ssl::stream_socket ssl_socket(ioc, ssl_ctx);
    lcy::system::error_code ec;

    make_connected_ssl_socket(ssl_ctx, ssl_socket);

    std::string req = "GET / HTTP/1.1\r\nHost: www.baidu.com\r\nConnection: close\r\n\r\n";
    std::string resp;
    resp.resize(4096);

    bool handshake_done = false;
    bool write_done = false;
    bool read_done = false;
    std::size_t write_n = 0;
    std::size_t read_n = 0;

    ssl_socket.async_client_handshake(
        [&](lcy::system::error_code ec) {
            expect_ec_ok(ec, "async handshake");
            handshake_done = true;

            ssl_socket.async_write_some(
                lcy::asio::buffer_view(&req[0], req.length()),
                [&](lcy::system::error_code ec, std::size_t n) {
                    expect_ec_ok(ec, "async write");
                    write_done = true;
                    write_n = n;

                    ssl_socket.async_read_some(
                        lcy::asio::buffer_view(&resp[0], resp.length()),
                        [&](lcy::system::error_code ec, std::size_t n) {
                            expect_ec_ok(ec, "async read");
                            read_done = true;
                            read_n = n;
                        });
                });
        });

    ioc.run();

    expect_true(handshake_done, "async handshake done");
    expect_true(write_done, "async write done");
    expect_size(write_n, req.length(), "async write n");
    expect_true(read_done, "async read done");
    expect_true(read_n > 0, "async read n > 0");

    ssl_socket.close(ec);
    expect_ec_ok(ec, "close");
    std::cout << "  ok\n";
}

static void test_async_write_then_close()
{
    std::cout << "[test_async_write_then_close]\n";
    lcy::asio::io_context ioc;
    lcy::asio::ssl::context ssl_ctx(lcy::asio::ssl::context::tls_client);
    lcy::asio::ssl::stream_socket ssl_socket(ioc, ssl_ctx);
    lcy::system::error_code ec;

    make_connected_ssl_socket(ssl_ctx, ssl_socket);

    std::string req = "GET / HTTP/1.1\r\nHost: www.baidu.com\r\nConnection: close\r\n\r\n";

    bool handshake_done = false;
    bool write_done = false;

    ssl_socket.async_client_handshake(
        [&](lcy::system::error_code ec) {
            expect_ec_ok(ec, "async handshake");
            handshake_done = true;

            ssl_socket.async_write_some(
                lcy::asio::buffer_view(&req[0], req.length()),
                [&](lcy::system::error_code ec, std::size_t) {
                    expect_ec_ok(ec, "async write");
                    write_done = true;
                });
        });

    ioc.run();

    expect_true(handshake_done, "async handshake done");
    expect_true(write_done, "async write done");

    ssl_socket.close(ec);
    expect_ec_ok(ec, "close");
    std::cout << "  ok\n";
}

static void test_async_handshake_after_close()
{
    std::cout << "[test_async_handshake_after_close]\n";
    lcy::asio::io_context ioc;
    lcy::asio::ssl::context ssl_ctx(lcy::asio::ssl::context::tls_client);
    lcy::asio::ssl::stream_socket ssl_socket(ioc, ssl_ctx);
    lcy::system::error_code ec;

    make_connected_ssl_socket(ssl_ctx, ssl_socket);
    ssl_socket.client_handshake(ec);
    expect_ec_ok(ec, "client_handshake");
    ssl_socket.close(ec);
    expect_ec_ok(ec, "close");

    bool called = false;
    lcy::system::error_code result_ec;
    ssl_socket.async_client_handshake(
        [&called, &result_ec](lcy::system::error_code ec) {
            called = true;
            result_ec = ec;
        });

    ioc.run();

    expect_true(called, "async handshake after close called");
    expect_ec_err(result_ec, "async handshake after close");
    std::cout << "  ok\n";
}

int main()
{
    test_set_hostname_and_verify_hostname();
    test_context_config();
    test_handshake_ok();
    test_write_read_some();
    test_write_read_throw_versions();
    test_short_read_write();
    test_handshake_twice();
    test_close_twice();
    test_write_after_close();
    test_read_after_close();
    test_handshake_without_connect();
    test_double_handshake_after_close();
    test_async_client_handshake();
    test_async_read_write_some();
    test_async_write_then_close();
    test_async_handshake_after_close();

    std::cout << "\nall ssl_stream_socket tests passed\n";
    return 0;
}