#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <unistd.h>

#include "lcy/asio/io_context.hpp"
#include "lcy/asio/ip/tcp.hpp"
#include "lcy/asio/strand.hpp"
#include "lcy/asio/buffer_view.hpp"
#include "lcy/asio/stream_read.hpp"
#include "lcy/asio/stream_write.hpp"
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

namespace {

typedef lcy::asio::ip::tcp::endpoint endpoint_type;
typedef lcy::asio::ip::tcp::socket tcp_socket_type;
typedef lcy::asio::ip::tcp::acceptor tcp_acceptor_type;
typedef lcy::asio::ssl::stream_socket ssl_socket_type;

const char server_cert_pem[] = R"(-----BEGIN CERTIFICATE-----
MIIDJzCCAg+gAwIBAgIUChFaNWvifHIMfjh4Re+UD/K0YpkwDQYJKoZIhvcNAQEL
BQAwFDESMBAGA1UEAwwJbG9jYWxob3N0MCAXDTI2MTEyNTE0NTk0NloYDzIxMjYx
MTAxMTQ1OTQ2WjAUMRIwEAYDVQQDDAlsb2NhbGhvc3QwggEiMA0GCSqGSIb3DQEB
AQUAA4IBDwAwggEKAoIBAQDoxxeLJ//LAmeY5b+mZBxvNx2NFUzU7lc4i0Qf/Ph+
xgJMT5/ZME2Lae9tKPxrwmfUJ9GgAIyGICGKz6flaFXlTagikkS6Orch/vmuASxa
Y8v4j1U3G/wxPb8p/B06S1qgk5oXALj50zbwU2bjc/jqp/sBa6V5blenqKHfIeoC
3TvpsX62QucaOB4hjJlJOCMaxQDxNa3sWavCfiujgux5vFPHBlOb65YjulYGF7Cv
mRBy47yLfVbZUl97zfxb18BJEXdLp0gIMFXncHrTYdI4QmIoZTj2nVLB2FNK7era
ne5bmWskOBjgns1TCjOw1j6PUAdIhHOMEsKSYB/D619nAgMBAAGjbzBtMB0GA1Ud
DgQWBBSW7X4EJy6pm54RuH9sd8UMIuPECTAfBgNVHSMEGDAWgBSW7X4EJy6pm54R
uH9sd8UMIuPECTAPBgNVHRMBAf8EBTADAQH/MBoGA1UdEQQTMBGCCWxvY2FsaG9z
dIcEfwAAATANBgkqhkiG9w0BAQsFAAOCAQEAz/eCCx4mMNExMdSvERAG66EDvhsu
Vw3pRhPFBWEUNnVf3eJqkIsE6hiSOKObmR3uN4l0gs4BGDoTj+GgCNahXsIepWjA
e1z8fIeNi9jhVTnFNjyzgeKBmRsXUQHX1uUZ7i1bp8FhjoqFmRz7qlMtAYWA7DdB
LLCTJ3c78E5Lq9gSs+gCfelbzVq+UE6p0YclBaVrFZwVY4N8tVbutxLQTRoixGQj
sh/LmrbCfmM/2dqZFsHjKCo8Rnh4gAI7RbOfBjfF89nbYib0JZnOQUChalmjqjSk
Qyj+OtnMginDJCSPZKhr1wNSSMNBs3IhVtSkEPgId7bafzf4j5jYs49cRQ==
-----END CERTIFICATE-----
)";

const char server_key_pem[] = R"(-----BEGIN PRIVATE KEY-----
MIIEvwIBADANBgkqhkiG9w0BAQEFAASCBKkwggSlAgEAAoIBAQDoxxeLJ//LAmeY
5b+mZBxvNx2NFUzU7lc4i0Qf/Ph+xgJMT5/ZME2Lae9tKPxrwmfUJ9GgAIyGICGK
z6flaFXlTagikkS6Orch/vmuASxaY8v4j1U3G/wxPb8p/B06S1qgk5oXALj50zbw
U2bjc/jqp/sBa6V5blenqKHfIeoC3TvpsX62QucaOB4hjJlJOCMaxQDxNa3sWavC
fiujgux5vFPHBlOb65YjulYGF7CvmRBy47yLfVbZUl97zfxb18BJEXdLp0gIMFXn
cHrTYdI4QmIoZTj2nVLB2FNK7erane5bmWskOBjgns1TCjOw1j6PUAdIhHOMEsKS
YB/D619nAgMBAAECggEAa17mM7hsCyKdUY/sEl6MJRqZpWmFSpRzo+/NKSohHyzL
g5KYy3A13JAtMb9di4EdLmV9kl2edAGfJy24pp81O7NR0jyAuhv6PBKl7Ya6BCmT
PKA0sNpWPgpawo15l9mJQJdE1MZvDEFKTplR1Km6T9bS92lFZxkoEnw8yjUVAWNU
eUPeoK5bBg0ICOy8+N4DZI2hq8XdaPsGNCFuH0Gx5DyEJPENnuC9MRLyi/zuVG+1
0eZxPrOHymPHB2hWwjZ4gJnb8vu41TZqqjfg2rVZhiPkY9IGmpHtW7KZE59j+dsm
RWwFuo2JtIX5jlz5OVy/ntVZJ+ZvCBp3ICVLLgUu2QKBgQD8pMoV3/ydVvbPIocd
JKVBh+y9RdeuqoMCX5HfF6PSiG9IJxo3itWcDntFlK4d05TztrhPjbsWkXetUFd+
EwSED48kgYen+X4PWFrKM1Lz8LVd0s7moBEN+XxdOyiNpC2F5ib7YBa0vD34cI5u
FRUTXIUacfnqzx7YSSwSPAT4XwKBgQDr3r2ePlT99iaPWEX8i91hiXyg5W+K2Y2a
GFs431J2gAsaEXMzC/asHEP97VU2bA++8ND+PQQ6yQdZ8Eo9TyDGlDZvsaxjAy5o
nTJ7fik2Q3lU32adXUe4epnz7yM+4FAZdZWSQgWJMzAu7qGhO7AFI9q7VlfvZL6R
4b98GQoV+QKBgQDcPxjBezPxQngFWXdqq6C5gTKcSn/dkF6HuiMj1qsFhdVllFkh
pJHV6zYCEby82dAXquoAyHEaGpS3bbvod9HPAvVqANXvVOxVmlQDxDyD1wt1kKq4
mUBk62NEr6cJ29+nxF2ldT42L4jixdUVGKH38AdFZ6lUz113dkrKVrKKhwKBgQDS
6v9MvhhqcO7SR/Ftf/Sa/8K+bexskVYacaVBGhI3fRBwYyJMh2oxRaVXMB9+5uq9
p8J7VjyuBI3aUxFP+a4pZOe4wXyRnYqmact2ur5Hk9avmoW8OmmkAZ4hj7pIw3Vd
DtSNURR6N5fin+lQsHsaXZF+7Br+K2xuLxx38DwgoQKBgQDJldFU+C/wq4xyGNom
C2z1QfbWrrHSbenXVNCIBgGNjzw2c8e7WIz6g6G5CWxMbW0iS8UvA9NipTq79d3g
/u4WEaMqMLzJOzYYxPcizDGYpa9FTqcFwy8gjmL05+SSA1Vc/afJNsOVsCARqW82
m+PVG2Kwc2nMlbTCgfgXtklxmw==
-----END PRIVATE KEY-----
)";

std::string g_cert_dir;
std::string g_cert_path;
std::string g_key_path;

void write_file(const std::string& path, const char* data)
{
    std::ofstream ofs(path.c_str(), std::ios::binary);
    expect_true(ofs.good(), "open cert file");
    ofs << data;
    expect_true(ofs.good(), "write cert file");
}

void setup_cert_files()
{
    char dir_template[] = "/tmp/lcy_ssl_local_XXXXXX";
    char* dir = ::mkdtemp(dir_template);
    expect_true(dir != 0, "mkdtemp");

    g_cert_dir = dir;
    g_cert_path = g_cert_dir + "/cert.pem";
    g_key_path = g_cert_dir + "/key.pem";

    write_file(g_cert_path, server_cert_pem);
    write_file(g_key_path, server_key_pem);
}

void cleanup_cert_files()
{
    if ( !g_cert_path.empty() ) {
        ::remove(g_cert_path.c_str());
    }
    if ( !g_key_path.empty() ) {
        ::remove(g_key_path.c_str());
    }
    if ( !g_cert_dir.empty() ) {
        ::rmdir(g_cert_dir.c_str());
    }
}

void configure_server_context(lcy::asio::ssl::context& ctx)
{
    lcy::system::error_code ec;
    ctx.use_certificate_file(g_cert_path,
        lcy::asio::ssl::context::pem, ec);
    expect_ec_ok(ec, "server use_certificate_file");

    ctx.use_private_key_file(g_key_path,
        lcy::asio::ssl::context::pem, ec);
    expect_ec_ok(ec, "server use_private_key_file");
}

void configure_client_verify(lcy::asio::ssl::context& ctx)
{
    lcy::system::error_code ec;
    ctx.set_verify_mode(lcy::asio::ssl::verify_peer);
    ctx.load_verify_file(g_cert_path, ec);
    expect_ec_ok(ec, "client load_verify_file");
}

endpoint_type loopback_endpoint(const endpoint_type& bound)
{
    std::string text = bound.to_string();
    std::size_t pos = text.rfind(':');
    expect_true(pos != std::string::npos, "endpoint to_string has port");

    unsigned long port = std::strtoul(text.c_str() + pos + 1, 0, 10);
    expect_true(port > 0 && port <= 65535, "endpoint port range");

    return endpoint_type(std::string("127.0.0.1"),
        static_cast<endpoint_type::port_type>(port));
}

void bind_acceptor(tcp_acceptor_type& acceptor, endpoint_type& bound)
{
    lcy::system::error_code ec;
    acceptor.open(lcy::asio::ip::tcp::v4(), ec);
    expect_ec_ok(ec, "acceptor open");

    acceptor.bind(endpoint_type(lcy::asio::ip::tcp::v4(), 0), ec);
    expect_ec_ok(ec, "acceptor bind");

    acceptor.listen(4, ec);
    expect_ec_ok(ec, "acceptor listen");

    acceptor.local_endpoint(bound, ec);
    expect_ec_ok(ec, "acceptor local_endpoint");
}

void connect_sync(ssl_socket_type& socket, const endpoint_type& ep,
    lcy::system::error_code& ec)
{
    socket.get_next_layer().open(lcy::asio::ip::tcp::v4(), ec);
    if ( ec ) {
        return;
    }

    socket.get_next_layer().connect(ep, ec);
}

void fill_pattern(std::vector<char>& buffer, std::uint32_t seed)
{
    for ( std::size_t i = 0; i < buffer.size(); ++i ) {
        buffer[i] = static_cast<char>((i * 31u + seed) & 0xffu);
    }
}

void expect_pattern(const std::vector<char>& buffer, std::uint32_t seed,
    const char* msg)
{
    for ( std::size_t i = 0; i < buffer.size(); ++i ) {
        if ( buffer[i] != static_cast<char>((i * 31u + seed) & 0xffu) ) {
            std::cerr << "FAIL: " << msg << " at offset " << i << "\n";
            std::abort();
        }
    }
}

typedef std::function<void(ssl_socket_type&)> server_script_type;

class sync_ssl_server
{
public:
    sync_ssl_server(lcy::asio::ssl::context& ctx, server_script_type script,
        bool do_handshake = true) :
        ioc_(), ctx_(ctx), acceptor_(ioc_), endpoint_(),
        script_(std::move(script)), do_handshake_(do_handshake),
        thread_(), accept_ec_(), handshake_ec_(),
        handshake_done_(false), script_ran_(false)
    {
        bind_acceptor(acceptor_, endpoint_);
        endpoint_ = loopback_endpoint(endpoint_);
    }

    ~sync_ssl_server()
    {
        join();
    }

    const endpoint_type& endpoint() const
    {
        return endpoint_;
    }

    void start()
    {
        thread_ = std::thread([this]() { run(); });
    }

    void join()
    {
        if ( thread_.joinable() ) {
            thread_.join();
        }
    }

    const lcy::system::error_code& accept_ec() const
    {
        return accept_ec_;
    }

    const lcy::system::error_code& handshake_ec() const
    {
        return handshake_ec_;
    }

    bool handshake_done() const
    {
        return handshake_done_;
    }

    bool script_ran() const
    {
        return script_ran_;
    }

private:
    void run()
    {
        ssl_socket_type socket(ioc_, ctx_);
        endpoint_type remote;

        acceptor_.accept(socket.get_next_layer(), remote, accept_ec_);
        if ( accept_ec_ ) {
            return;
        }

        if ( do_handshake_ ) {
            socket.server_handshake(handshake_ec_);
            if ( handshake_ec_ ) {
                return;
            }
            handshake_done_ = true;
        }

        script_ran_ = true;
        script_(socket);
    }

    lcy::asio::io_context ioc_;
    lcy::asio::ssl::context& ctx_;
    tcp_acceptor_type acceptor_;
    endpoint_type endpoint_;
    server_script_type script_;
    bool do_handshake_;
    std::thread thread_;
    lcy::system::error_code accept_ec_;
    lcy::system::error_code handshake_ec_;
    bool handshake_done_;
    bool script_ran_;
};

class plain_tcp_server
{
public:
    enum mode
    {
        garbage_mode,
        close_mode
    };

    explicit plain_tcp_server(mode m) :
        ioc_(), acceptor_(ioc_), endpoint_(), mode_(m),
        thread_(), accept_ec_()
    {
        bind_acceptor(acceptor_, endpoint_);
        endpoint_ = loopback_endpoint(endpoint_);
    }

    ~plain_tcp_server()
    {
        join();
    }

    const endpoint_type& endpoint() const
    {
        return endpoint_;
    }

    void start()
    {
        thread_ = std::thread([this]() { run(); });
    }

    void join()
    {
        if ( thread_.joinable() ) {
            thread_.join();
        }
    }

    const lcy::system::error_code& accept_ec() const
    {
        return accept_ec_;
    }

private:
    void run()
    {
        tcp_socket_type socket(ioc_);
        endpoint_type remote;

        acceptor_.accept(socket, remote, accept_ec_);
        if ( accept_ec_ ) {
            return;
        }

        lcy::system::error_code ec;
        if ( mode_ == garbage_mode ) {
            const char message[] = "this is definitely not a tls server\r\n";
            socket.write_some(
                lcy::asio::buffer_view(message, sizeof(message) - 1), ec);
        }

        socket.close(ec);
    }

    lcy::asio::io_context ioc_;
    tcp_acceptor_type acceptor_;
    endpoint_type endpoint_;
    mode mode_;
    std::thread thread_;
    lcy::system::error_code accept_ec_;
};

void server_echo_once(ssl_socket_type& socket, std::size_t size)
{
    std::vector<char> buffer(size);
    lcy::system::error_code ec;

    std::size_t n = lcy::asio::read(socket,
        lcy::asio::buffer_view(buffer.data(), buffer.size()), ec);
    if ( ec ) {
        return;
    }
    expect_size(n, size, "server read echo payload");

    lcy::asio::write(socket,
        lcy::asio::buffer_view(buffer.data(), buffer.size()), ec);
}

void server_echo_framed(ssl_socket_type& socket)
{
    lcy::system::error_code ec;

    for ( ;; ) {
        char header[8] = { 0 };
        std::size_t n = lcy::asio::read(socket,
            lcy::asio::buffer_view(header, sizeof(header)), ec);
        if ( ec ) {
            return;
        }
        expect_size(n, sizeof(header), "server read length header");

        std::uint64_t size = 0;
        std::memcpy(&size, header, sizeof(size));

        if ( size == 0 ) {
            return;
        }

        std::vector<char> buffer(static_cast<std::size_t>(size));
        n = lcy::asio::read(socket,
            lcy::asio::buffer_view(buffer.data(), buffer.size()), ec);
        if ( ec ) {
            return;
        }
        expect_size(n, buffer.size(), "server read framed payload");

        lcy::asio::write(socket,
            lcy::asio::buffer_view(buffer.data(), buffer.size()), ec);
        if ( ec ) {
            return;
        }
    }
}

void server_read_until_error(ssl_socket_type& socket)
{
    lcy::system::error_code ec;
    char buffer[256];

    while ( !ec ) {
        socket.read_some(lcy::asio::buffer_view(buffer, sizeof(buffer)), ec);
    }
}

void test_local_handshake_sync()
{
    std::cout << "[test_local_handshake_sync]\n";

    lcy::asio::ssl::context server_ctx(lcy::asio::ssl::context::tls_server);
    configure_server_context(server_ctx);

    lcy::asio::ssl::context client_ctx(lcy::asio::ssl::context::tls_client);

    sync_ssl_server server(server_ctx,
        [](ssl_socket_type&) {});
    server.start();

    lcy::asio::io_context ioc;
    ssl_socket_type client(ioc, client_ctx);

    lcy::system::error_code ec;
    connect_sync(client, server.endpoint(), ec);
    expect_ec_ok(ec, "client connect");

    client.client_handshake(ec);
    expect_ec_ok(ec, "client handshake");
    expect_true(client.get_next_layer().is_open(), "client open");

    client.close(ec);
    expect_ec_ok(ec, "client close");

    server.join();
    expect_ec_ok(server.accept_ec(), "server accept");
    expect_ec_ok(server.handshake_ec(), "server handshake");
    expect_true(server.handshake_done(), "server handshake done");

    std::cout << "  ok\n";
}

void test_local_handshake_verify_peer_ok()
{
    std::cout << "[test_local_handshake_verify_peer_ok]\n";

    lcy::asio::ssl::context server_ctx(lcy::asio::ssl::context::tls_server);
    configure_server_context(server_ctx);

    lcy::asio::ssl::context client_ctx(lcy::asio::ssl::context::tls_client);
    configure_client_verify(client_ctx);

    sync_ssl_server server(server_ctx,
        [](ssl_socket_type&) {});
    server.start();

    lcy::asio::io_context ioc;
    ssl_socket_type client(ioc, client_ctx);

    lcy::system::error_code ec;
    client.set_hostname("localhost", ec);
    expect_ec_ok(ec, "client set_hostname");
    client.set_verify_hostname("localhost", ec);
    expect_ec_ok(ec, "client set_verify_hostname");

    connect_sync(client, server.endpoint(), ec);
    expect_ec_ok(ec, "client connect");

    client.client_handshake(ec);
    expect_ec_ok(ec, "client handshake with peer verification");

    client.close(ec);
    expect_ec_ok(ec, "client close");

    server.join();
    expect_ec_ok(server.handshake_ec(), "server handshake");

    std::cout << "  ok\n";
}

void test_local_handshake_verify_peer_no_ca()
{
    std::cout << "[test_local_handshake_verify_peer_no_ca]\n";

    lcy::asio::ssl::context server_ctx(lcy::asio::ssl::context::tls_server);
    configure_server_context(server_ctx);

    lcy::asio::ssl::context client_ctx(lcy::asio::ssl::context::tls_client);
    client_ctx.set_verify_mode(lcy::asio::ssl::verify_peer);

    sync_ssl_server server(server_ctx,
        [](ssl_socket_type&) {});
    server.start();

    lcy::asio::io_context ioc;
    ssl_socket_type client(ioc, client_ctx);

    lcy::system::error_code ec;
    connect_sync(client, server.endpoint(), ec);
    expect_ec_ok(ec, "client connect");

    client.client_handshake(ec);
    expect_ec_err(ec, "client handshake without trusted ca");

    lcy::system::error_code ignored_ec;
    client.close(ignored_ec);
    server.join();

    std::cout << "  ok\n";
}

void test_local_handshake_hostname_mismatch()
{
    std::cout << "[test_local_handshake_hostname_mismatch]\n";

    lcy::asio::ssl::context server_ctx(lcy::asio::ssl::context::tls_server);
    configure_server_context(server_ctx);

    lcy::asio::ssl::context client_ctx(lcy::asio::ssl::context::tls_client);
    configure_client_verify(client_ctx);

    sync_ssl_server server(server_ctx,
        [](ssl_socket_type&) {});
    server.start();

    lcy::asio::io_context ioc;
    ssl_socket_type client(ioc, client_ctx);

    lcy::system::error_code ec;
    client.set_hostname("wrong.example", ec);
    expect_ec_ok(ec, "client set_hostname");
    client.set_verify_hostname("wrong.example", ec);
    expect_ec_ok(ec, "client set_verify_hostname");

    connect_sync(client, server.endpoint(), ec);
    expect_ec_ok(ec, "client connect");

    client.client_handshake(ec);
    expect_ec_err(ec, "client handshake with hostname mismatch");

    lcy::system::error_code ignored_ec;
    client.close(ignored_ec);
    server.join();

    std::cout << "  ok\n";
}

void test_server_handshake_without_cert()
{
    std::cout << "[test_server_handshake_without_cert]\n";

    lcy::asio::ssl::context server_ctx(lcy::asio::ssl::context::tls_server);
    lcy::asio::ssl::context client_ctx(lcy::asio::ssl::context::tls_client);

    sync_ssl_server server(server_ctx,
        [](ssl_socket_type&) {});
    server.start();

    lcy::asio::io_context ioc;
    ssl_socket_type client(ioc, client_ctx);

    lcy::system::error_code ec;
    connect_sync(client, server.endpoint(), ec);
    expect_ec_ok(ec, "client connect");

    client.client_handshake(ec);
    expect_ec_err(ec, "client handshake against server without cert");

    server.join();
    expect_ec_err(server.handshake_ec(), "server handshake without cert");
    expect_true(!server.handshake_done(), "server handshake not done");
    expect_true(!server.script_ran(), "server script not run");

    std::cout << "  ok\n";
}

void test_client_handshake_against_plain_server()
{
    std::cout << "[test_client_handshake_against_plain_server]\n";

    plain_tcp_server server(plain_tcp_server::garbage_mode);
    server.start();

    lcy::asio::ssl::context client_ctx(lcy::asio::ssl::context::tls_client);
    lcy::asio::io_context ioc;
    ssl_socket_type client(ioc, client_ctx);

    lcy::system::error_code ec;
    connect_sync(client, server.endpoint(), ec);
    expect_ec_ok(ec, "client connect");

    client.client_handshake(ec);
    expect_ec_err(ec, "client handshake against plain server");

    server.join();
    expect_ec_ok(server.accept_ec(), "plain server accept");

    std::cout << "  ok\n";
}

void test_server_handshake_against_plain_client()
{
    std::cout << "[test_server_handshake_against_plain_client]\n";

    lcy::asio::ssl::context server_ctx(lcy::asio::ssl::context::tls_server);
    configure_server_context(server_ctx);

    sync_ssl_server server(server_ctx,
        [](ssl_socket_type&) {});
    server.start();

    lcy::asio::io_context ioc;
    tcp_socket_type client(ioc);

    lcy::system::error_code ec;
    client.open(lcy::asio::ip::tcp::v4(), ec);
    expect_ec_ok(ec, "plain client open");

    client.connect(server.endpoint(), ec);
    expect_ec_ok(ec, "plain client connect");

    const char message[] = "plain text, not tls";
    client.write_some(
        lcy::asio::buffer_view(message, sizeof(message) - 1), ec);
    expect_ec_ok(ec, "plain client write");
    client.close(ec);

    server.join();
    expect_ec_ok(server.accept_ec(), "server accept");
    expect_ec_err(server.handshake_ec(), "server handshake against plain client");
    expect_true(!server.handshake_done(), "server handshake not done");

    std::cout << "  ok\n";
}

void test_handshake_version_mismatch()
{
    std::cout << "[test_handshake_version_mismatch]\n";

    lcy::asio::ssl::context server_ctx(lcy::asio::ssl::context::tls_server);
    configure_server_context(server_ctx);
    server_ctx.set_min_proto_version(TLS1_3_VERSION);

    lcy::asio::ssl::context client_ctx(lcy::asio::ssl::context::tls_client);
    client_ctx.set_max_proto_version(TLS1_2_VERSION);

    sync_ssl_server server(server_ctx,
        [](ssl_socket_type&) {});
    server.start();

    lcy::asio::io_context ioc;
    ssl_socket_type client(ioc, client_ctx);

    lcy::system::error_code ec;
    connect_sync(client, server.endpoint(), ec);
    expect_ec_ok(ec, "client connect");

    client.client_handshake(ec);
    expect_ec_err(ec, "client handshake with version mismatch");

    server.join();
    expect_ec_err(server.handshake_ec(), "server handshake with version mismatch");
    expect_true(!server.handshake_done(), "server handshake not done");

    std::cout << "  ok\n";
}

void test_handshake_twice_local()
{
    std::cout << "[test_handshake_twice_local]\n";

    lcy::asio::ssl::context server_ctx(lcy::asio::ssl::context::tls_server);
    configure_server_context(server_ctx);

    lcy::asio::ssl::context client_ctx(lcy::asio::ssl::context::tls_client);

    sync_ssl_server server(server_ctx, server_read_until_error);
    server.start();

    lcy::asio::io_context ioc;
    ssl_socket_type client(ioc, client_ctx);

    lcy::system::error_code ec;
    connect_sync(client, server.endpoint(), ec);
    expect_ec_ok(ec, "client connect");

    client.client_handshake(ec);
    expect_ec_ok(ec, "client first handshake");

    client.client_handshake(ec);
    expect_ec_ok(ec, "client second handshake");

    client.close(ec);
    expect_ec_ok(ec, "client close");

    server.join();
    expect_ec_ok(server.handshake_ec(), "server first handshake");

    std::cout << "  ok\n";
}

void test_handshake_after_close_local()
{
    std::cout << "[test_handshake_after_close_local]\n";

    lcy::asio::ssl::context server_ctx(lcy::asio::ssl::context::tls_server);
    configure_server_context(server_ctx);

    lcy::asio::ssl::context client_ctx(lcy::asio::ssl::context::tls_client);

    sync_ssl_server server(server_ctx, server_read_until_error);
    server.start();

    lcy::asio::io_context ioc;
    ssl_socket_type client(ioc, client_ctx);

    lcy::system::error_code ec;
    connect_sync(client, server.endpoint(), ec);
    expect_ec_ok(ec, "client connect");

    client.client_handshake(ec);
    expect_ec_ok(ec, "client handshake");

    client.close(ec);
    expect_ec_ok(ec, "client close");

    client.client_handshake(ec);
    expect_ec_err(ec, "client handshake after close");

    server.join();
    expect_ec_ok(server.handshake_ec(), "server handshake");

    std::cout << "  ok\n";
}

void test_write_after_close_local()
{
    std::cout << "[test_write_after_close_local]\n";

    lcy::asio::ssl::context server_ctx(lcy::asio::ssl::context::tls_server);
    configure_server_context(server_ctx);

    lcy::asio::ssl::context client_ctx(lcy::asio::ssl::context::tls_client);

    sync_ssl_server server(server_ctx, server_read_until_error);
    server.start();

    lcy::asio::io_context ioc;
    ssl_socket_type client(ioc, client_ctx);

    lcy::system::error_code ec;
    connect_sync(client, server.endpoint(), ec);
    expect_ec_ok(ec, "client connect");

    client.client_handshake(ec);
    expect_ec_ok(ec, "client handshake");

    client.close(ec);
    expect_ec_ok(ec, "client close");

    const char message[] = "after close";
    std::size_t n = client.write_some(
        lcy::asio::buffer_view(message, sizeof(message) - 1), ec);
    expect_ec_err(ec, "client write after close");
    expect_size(n, 0, "client write after close bytes");

    char buffer[8];
    n = client.read_some(lcy::asio::buffer_view(buffer, sizeof(buffer)), ec);
    expect_ec_err(ec, "client read after close");
    expect_size(n, 0, "client read after close bytes");

    server.join();

    std::cout << "  ok\n";
}

void test_close_twice_local()
{
    std::cout << "[test_close_twice_local]\n";

    lcy::asio::ssl::context server_ctx(lcy::asio::ssl::context::tls_server);
    configure_server_context(server_ctx);

    lcy::asio::ssl::context client_ctx(lcy::asio::ssl::context::tls_client);

    sync_ssl_server server(server_ctx, server_read_until_error);
    server.start();

    lcy::asio::io_context ioc;
    ssl_socket_type client(ioc, client_ctx);

    lcy::system::error_code ec;
    connect_sync(client, server.endpoint(), ec);
    expect_ec_ok(ec, "client connect");

    client.client_handshake(ec);
    expect_ec_ok(ec, "client handshake");

    client.close(ec);
    expect_ec_ok(ec, "client first close");

    client.close(ec);
    expect_ec_err(ec, "client second close");

    server.join();

    std::cout << "  ok\n";
}

void test_write_read_large_sync()
{
    std::cout << "[test_write_read_large_sync]\n";

    const std::size_t payload_size = 1024 * 1024;

    lcy::asio::ssl::context server_ctx(lcy::asio::ssl::context::tls_server);
    configure_server_context(server_ctx);

    lcy::asio::ssl::context client_ctx(lcy::asio::ssl::context::tls_client);

    sync_ssl_server server(server_ctx,
        [payload_size](ssl_socket_type& socket) {
            std::vector<char> buffer(payload_size);
            lcy::system::error_code ec;

            std::size_t n = lcy::asio::read(socket,
                lcy::asio::buffer_view(buffer.data(), buffer.size()), ec);
            if ( ec ) {
                return;
            }
            expect_size(n, payload_size, "server read large payload");
            expect_pattern(buffer, 7, "server received large payload");

            lcy::asio::write(socket,
                lcy::asio::buffer_view(buffer.data(), buffer.size()), ec);
        });
    server.start();

    lcy::asio::io_context ioc;
    ssl_socket_type client(ioc, client_ctx);

    lcy::system::error_code ec;
    connect_sync(client, server.endpoint(), ec);
    expect_ec_ok(ec, "client connect");

    client.client_handshake(ec);
    expect_ec_ok(ec, "client handshake");

    std::vector<char> payload(payload_size);
    fill_pattern(payload, 7);

    std::size_t n = lcy::asio::write(client,
        lcy::asio::buffer_view(payload.data(), payload.size()), ec);
    expect_ec_ok(ec, "client write large payload");
    expect_size(n, payload_size, "client write large payload size");

    std::vector<char> echo(payload_size);
    n = lcy::asio::read(client,
        lcy::asio::buffer_view(echo.data(), echo.size()), ec);
    expect_ec_ok(ec, "client read large echo");
    expect_size(n, payload_size, "client read large echo size");
    expect_pattern(echo, 7, "client received large echo");

    client.close(ec);
    expect_ec_ok(ec, "client close");

    server.join();

    std::cout << "  ok\n";
}

void test_chunk_size_boundaries()
{
    std::cout << "[test_chunk_size_boundaries]\n";

    const std::size_t sizes[] = {
        1, 2, 15, 16, 17, 255, 256, 257,
        4095, 4096, 4097,
        16383, 16384, 16385,
        65535, 65536, 65537,
        262144
    };

    lcy::asio::ssl::context server_ctx(lcy::asio::ssl::context::tls_server);
    configure_server_context(server_ctx);

    lcy::asio::ssl::context client_ctx(lcy::asio::ssl::context::tls_client);

    sync_ssl_server server(server_ctx, server_echo_framed);
    server.start();

    lcy::asio::io_context ioc;
    ssl_socket_type client(ioc, client_ctx);

    lcy::system::error_code ec;
    connect_sync(client, server.endpoint(), ec);
    expect_ec_ok(ec, "client connect");

    client.client_handshake(ec);
    expect_ec_ok(ec, "client handshake");

    for ( std::size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i ) {
        const std::size_t size = sizes[i];

        std::uint64_t header_value =
            static_cast<std::uint64_t>(size);
        char header[8] = { 0 };
        std::memcpy(header, &header_value, sizeof(header_value));

        std::vector<char> payload(size);
        fill_pattern(payload, static_cast<std::uint32_t>(i));

        std::size_t n = lcy::asio::write(client,
            lcy::asio::buffer_view(header, sizeof(header)), ec);
        expect_ec_ok(ec, "client write chunk header");
        expect_size(n, sizeof(header), "client write chunk header size");

        n = lcy::asio::write(client,
            lcy::asio::buffer_view(payload.data(), payload.size()), ec);
        expect_ec_ok(ec, "client write chunk payload");
        expect_size(n, payload.size(), "client write chunk payload size");

        std::vector<char> echo(size);
        n = lcy::asio::read(client,
            lcy::asio::buffer_view(echo.data(), echo.size()), ec);
        expect_ec_ok(ec, "client read chunk echo");
        expect_size(n, size, "client read chunk echo size");
        expect_pattern(echo, static_cast<std::uint32_t>(i),
            "client chunk echo pattern");
    }

    std::uint64_t terminate = 0;
    char header[8] = { 0 };
    std::memcpy(header, &terminate, sizeof(terminate));
    lcy::asio::write(client,
        lcy::asio::buffer_view(header, sizeof(header)), ec);
    expect_ec_ok(ec, "client write chunk terminator");

    client.close(ec);
    expect_ec_ok(ec, "client close");

    server.join();

    std::cout << "  ok\n";
}

void test_read_short_buffer()
{
    std::cout << "[test_read_short_buffer]\n";

    const std::size_t message_size = 100;

    lcy::asio::ssl::context server_ctx(lcy::asio::ssl::context::tls_server);
    configure_server_context(server_ctx);

    lcy::asio::ssl::context client_ctx(lcy::asio::ssl::context::tls_client);

    sync_ssl_server server(server_ctx,
        [message_size](ssl_socket_type& socket) {
            std::vector<char> message(message_size);
            fill_pattern(message, 3);

            lcy::system::error_code ec;
            lcy::asio::write(socket,
                lcy::asio::buffer_view(message.data(), message.size()), ec);
        });
    server.start();

    lcy::asio::io_context ioc;
    ssl_socket_type client(ioc, client_ctx);

    lcy::system::error_code ec;
    connect_sync(client, server.endpoint(), ec);
    expect_ec_ok(ec, "client connect");

    client.client_handshake(ec);
    expect_ec_ok(ec, "client handshake");

    std::vector<char> received(message_size);
    std::size_t total = 0;

    while ( total < message_size ) {
        char byte = 0;
        std::size_t n = client.read_some(
            lcy::asio::buffer_view(&byte, 1), ec);
        expect_ec_ok(ec, "client short read");
        expect_true(n == 1, "client short read returns one byte");
        received[total] = byte;
        total += n;
    }

    expect_pattern(received, 3, "client short read pattern");

    client.close(ec);
    expect_ec_ok(ec, "client close");

    server.join();

    std::cout << "  ok\n";
}

void test_peer_close_notify()
{
    std::cout << "[test_peer_close_notify]\n";

    lcy::asio::ssl::context server_ctx(lcy::asio::ssl::context::tls_server);
    configure_server_context(server_ctx);

    lcy::asio::ssl::context client_ctx(lcy::asio::ssl::context::tls_client);

    sync_ssl_server server(server_ctx,
        [](ssl_socket_type& socket) {
            lcy::system::error_code ec;
            socket.close(ec);
            expect_ec_ok(ec, "server close with close_notify");
        });
    server.start();

    lcy::asio::io_context ioc;
    ssl_socket_type client(ioc, client_ctx);

    lcy::system::error_code ec;
    connect_sync(client, server.endpoint(), ec);
    expect_ec_ok(ec, "client connect");

    client.client_handshake(ec);
    expect_ec_ok(ec, "client handshake");

    char buffer[16];
    std::size_t n = client.read_some(
        lcy::asio::buffer_view(buffer, sizeof(buffer)), ec);
    expect_ec_err(ec, "client read after peer close_notify");
    expect_size(n, 0, "client read after peer close_notify bytes");

    server.join();

    std::cout << "  ok\n";
}

void test_peer_abrupt_close()
{
    std::cout << "[test_peer_abrupt_close]\n";

    lcy::asio::ssl::context server_ctx(lcy::asio::ssl::context::tls_server);
    configure_server_context(server_ctx);

    lcy::asio::ssl::context client_ctx(lcy::asio::ssl::context::tls_client);

    sync_ssl_server server(server_ctx,
        [](ssl_socket_type& socket) {
            lcy::system::error_code ec;
            socket.get_next_layer().close(ec);
            expect_ec_ok(ec, "server abrupt tcp close");
        });
    server.start();

    lcy::asio::io_context ioc;
    ssl_socket_type client(ioc, client_ctx);

    lcy::system::error_code ec;
    connect_sync(client, server.endpoint(), ec);
    expect_ec_ok(ec, "client connect");

    client.client_handshake(ec);
    expect_ec_ok(ec, "client handshake");

    char buffer[16];
    std::size_t n = client.read_some(
        lcy::asio::buffer_view(buffer, sizeof(buffer)), ec);
    expect_ec_err(ec, "client read after abrupt tcp close");
    expect_size(n, 0, "client read after abrupt tcp close bytes");

    server.join();

    std::cout << "  ok\n";
}

void test_get_strand_post()
{
    std::cout << "[test_get_strand_post]\n";

    lcy::asio::io_context ioc;
    lcy::asio::ssl::context ctx(lcy::asio::ssl::context::tls_client);
    ssl_socket_type socket(ioc, ctx);

    bool called = false;
    socket.get_strand().post([&called]() {
        called = true;
    });

    ioc.run();
    expect_true(called, "posted function on ssl strand");

    std::cout << "  ok\n";
}

void test_async_client_handshake_local()
{
    std::cout << "[test_async_client_handshake_local]\n";

    const std::size_t message_size = 16;

    lcy::asio::ssl::context server_ctx(lcy::asio::ssl::context::tls_server);
    configure_server_context(server_ctx);

    lcy::asio::ssl::context client_ctx(lcy::asio::ssl::context::tls_client);

    sync_ssl_server server(server_ctx,
        [message_size](ssl_socket_type& socket) {
            server_echo_once(socket, message_size);
        });
    server.start();

    lcy::asio::io_context ioc;
    ssl_socket_type client(ioc, client_ctx);

    lcy::system::error_code ec;
    connect_sync(client, server.endpoint(), ec);
    expect_ec_ok(ec, "client connect");

    std::vector<char> request(message_size);
    fill_pattern(request, 11);
    std::vector<char> response(message_size);

    bool handshake_done = false;
    bool write_done = false;
    bool read_done = false;

    client.async_client_handshake(
        [&](lcy::system::error_code handshake_ec) {
            expect_ec_ok(handshake_ec, "async client handshake");
            handshake_done = true;

            lcy::asio::async_write(client,
                lcy::asio::buffer_view(request.data(), request.size()),
                [&](lcy::system::error_code write_ec, std::size_t n) {
                    expect_ec_ok(write_ec, "async client write");
                    expect_size(n, request.size(), "async client write size");
                    write_done = true;

                    lcy::asio::async_read(client,
                        lcy::asio::buffer_view(response.data(),
                            response.size()),
                        [&](lcy::system::error_code read_ec, std::size_t rn) {
                            expect_ec_ok(read_ec, "async client read");
                            expect_size(rn, response.size(),
                                "async client read size");
                            read_done = true;
                        });
                });
        });

    ioc.run();

    expect_true(handshake_done, "async handshake done");
    expect_true(write_done, "async write done");
    expect_true(read_done, "async read done");
    expect_pattern(response, 11, "async client echo pattern");

    client.close(ec);
    expect_ec_ok(ec, "client close");

    server.join();

    std::cout << "  ok\n";
}

void test_async_server_handshake_local()
{
    std::cout << "[test_async_server_handshake_local]\n";

    const std::size_t message_size = 16;

    lcy::asio::ssl::context server_ctx(lcy::asio::ssl::context::tls_server);
    configure_server_context(server_ctx);

    lcy::asio::ssl::context client_ctx(lcy::asio::ssl::context::tls_client);

    lcy::asio::io_context server_ioc;
    tcp_acceptor_type acceptor(server_ioc);
    endpoint_type bound;
    bind_acceptor(acceptor, bound);
    endpoint_type server_endpoint = loopback_endpoint(bound);

    bool server_handshake_done = false;
    bool server_read_done = false;
    bool server_write_done = false;
    std::vector<char> server_request(message_size);

    std::thread server_thread([&]() {
        ssl_socket_type server_socket(server_ioc, server_ctx);

        lcy::system::error_code ec;
        endpoint_type remote;
        acceptor.accept(server_socket.get_next_layer(), remote, ec);
        expect_ec_ok(ec, "async server accept");

        server_socket.async_server_handshake(
            [&](lcy::system::error_code handshake_ec) {
                expect_ec_ok(handshake_ec, "async server handshake");
                server_handshake_done = true;

                lcy::asio::async_read(server_socket,
                    lcy::asio::buffer_view(server_request.data(),
                        server_request.size()),
                    [&](lcy::system::error_code read_ec, std::size_t n) {
                        expect_ec_ok(read_ec, "async server read");
                        expect_size(n, server_request.size(),
                            "async server read size");
                        server_read_done = true;

                        lcy::asio::async_write(server_socket,
                            lcy::asio::buffer_view(server_request.data(),
                                server_request.size()),
                            [&](lcy::system::error_code write_ec,
                                std::size_t wn) {
                                expect_ec_ok(write_ec, "async server write");
                                expect_size(wn, server_request.size(),
                                    "async server write size");
                                server_write_done = true;
                            });
                    });
            });

        server_ioc.run();
    });

    lcy::asio::io_context client_ioc;
    ssl_socket_type client(client_ioc, client_ctx);

    lcy::system::error_code ec;
    connect_sync(client, server_endpoint, ec);
    expect_ec_ok(ec, "client connect");

    client.client_handshake(ec);
    expect_ec_ok(ec, "client handshake");

    std::vector<char> request(message_size);
    fill_pattern(request, 21);

    std::size_t n = lcy::asio::write(client,
        lcy::asio::buffer_view(request.data(), request.size()), ec);
    expect_ec_ok(ec, "client write");
    expect_size(n, request.size(), "client write size");

    std::vector<char> response(message_size);
    n = lcy::asio::read(client,
        lcy::asio::buffer_view(response.data(), response.size()), ec);
    expect_ec_ok(ec, "client read");
    expect_size(n, response.size(), "client read size");
    expect_pattern(response, 21, "async server echo pattern");

    client.close(ec);
    expect_ec_ok(ec, "client close");

    server_thread.join();

    expect_true(server_handshake_done, "async server handshake done");
    expect_true(server_read_done, "async server read done");
    expect_true(server_write_done, "async server write done");

    std::cout << "  ok\n";
}

void test_async_full_duplex_multithread()
{
    std::cout << "[test_async_full_duplex_multithread]\n";

    const std::size_t payload_size = 256 * 1024;

    lcy::asio::ssl::context server_ctx(lcy::asio::ssl::context::tls_server);
    configure_server_context(server_ctx);

    lcy::asio::ssl::context client_ctx(lcy::asio::ssl::context::tls_client);

    sync_ssl_server server(server_ctx,
        [payload_size](ssl_socket_type& socket) {
            std::vector<char> request(payload_size);
            lcy::system::error_code ec;

            std::size_t n = lcy::asio::read(socket,
                lcy::asio::buffer_view(request.data(), request.size()), ec);
            if ( ec ) {
                return;
            }
            expect_size(n, payload_size, "server read duplex payload");
            expect_pattern(request, 31, "server duplex payload pattern");

            lcy::asio::write(socket,
                lcy::asio::buffer_view(request.data(), request.size()), ec);
        });
    server.start();

    lcy::asio::io_context ioc(2);
    ssl_socket_type client(ioc, client_ctx);

    lcy::system::error_code ec;
    connect_sync(client, server.endpoint(), ec);
    expect_ec_ok(ec, "client connect");

    std::vector<char> request(payload_size);
    fill_pattern(request, 31);
    std::vector<char> response(payload_size);

    bool handshake_done = false;
    bool write_done = false;
    bool read_done = false;

    client.async_client_handshake(
        [&](lcy::system::error_code handshake_ec) {
            expect_ec_ok(handshake_ec, "async duplex handshake");
            handshake_done = true;

            lcy::asio::async_write(client,
                lcy::asio::buffer_view(request.data(), request.size()),
                [&](lcy::system::error_code write_ec, std::size_t n) {
                    expect_ec_ok(write_ec, "async duplex write");
                    expect_size(n, request.size(), "async duplex write size");
                    write_done = true;
                });

            lcy::asio::async_read(client,
                lcy::asio::buffer_view(response.data(), response.size()),
                [&](lcy::system::error_code read_ec, std::size_t n) {
                    expect_ec_ok(read_ec, "async duplex read");
                    expect_size(n, response.size(), "async duplex read size");
                    read_done = true;
                });
        });

    std::thread runner1([&]() { ioc.run(); });
    std::thread runner2([&]() { ioc.run(); });
    runner1.join();
    runner2.join();

    expect_true(handshake_done, "async duplex handshake done");
    expect_true(write_done, "async duplex write done");
    expect_true(read_done, "async duplex read done");
    expect_pattern(response, 31, "async duplex echo pattern");

    client.close(ec);
    expect_ec_ok(ec, "client close");

    server.join();

    std::cout << "  ok\n";
}

void test_async_multiple_writes_via_strand()
{
    std::cout << "[test_async_multiple_writes_via_strand]\n";

    const std::string first("abc");
    const std::string second("def");

    lcy::asio::ssl::context server_ctx(lcy::asio::ssl::context::tls_server);
    configure_server_context(server_ctx);

    lcy::asio::ssl::context client_ctx(lcy::asio::ssl::context::tls_client);

    sync_ssl_server server(server_ctx,
        [first, second](ssl_socket_type& socket) {
            const std::size_t total = first.size() + second.size();
            std::vector<char> received(total);
            lcy::system::error_code ec;

            std::size_t n = lcy::asio::read(socket,
                lcy::asio::buffer_view(received.data(), received.size()), ec);
            if ( ec ) {
                return;
            }
            expect_size(n, total, "server read ordered payload");

            std::string text(received.begin(), received.end());
            expect_true(text == first + second,
                "server received writes in strand order");
        });
    server.start();

    lcy::asio::io_context ioc;
    ssl_socket_type client(ioc, client_ctx);

    lcy::system::error_code ec;
    connect_sync(client, server.endpoint(), ec);
    expect_ec_ok(ec, "client connect");

    bool handshake_done = false;
    bool first_done = false;
    bool second_done = false;

    client.async_client_handshake(
        [&](lcy::system::error_code handshake_ec) {
            expect_ec_ok(handshake_ec, "async ordered handshake");
            handshake_done = true;

            lcy::asio::strand& strand = client.get_strand();

            strand.post([&]() {
                client.async_write_some(
                    lcy::asio::buffer_view(first.data(), first.size()),
                    [&](lcy::system::error_code write_ec, std::size_t n) {
                        expect_ec_ok(write_ec, "async ordered first write");
                        expect_size(n, first.size(),
                            "async ordered first write size");
                        first_done = true;
                    });
            });

            strand.post([&]() {
                client.async_write_some(
                    lcy::asio::buffer_view(second.data(), second.size()),
                    [&](lcy::system::error_code write_ec, std::size_t n) {
                        expect_ec_ok(write_ec, "async ordered second write");
                        expect_size(n, second.size(),
                            "async ordered second write size");
                        second_done = true;
                    });
            });
        });

    ioc.run();

    expect_true(handshake_done, "async ordered handshake done");
    expect_true(first_done, "async ordered first write done");
    expect_true(second_done, "async ordered second write done");

    client.close(ec);
    expect_ec_ok(ec, "client close");

    server.join();

    std::cout << "  ok\n";
}

void test_async_handshake_failure_plain_server()
{
    std::cout << "[test_async_handshake_failure_plain_server]\n";

    plain_tcp_server server(plain_tcp_server::close_mode);
    server.start();

    lcy::asio::ssl::context client_ctx(lcy::asio::ssl::context::tls_client);
    lcy::asio::io_context ioc;
    ssl_socket_type client(ioc, client_ctx);

    lcy::system::error_code ec;
    connect_sync(client, server.endpoint(), ec);
    expect_ec_ok(ec, "client connect");

    bool called = false;
    lcy::system::error_code handshake_ec;

    client.async_client_handshake(
        [&](lcy::system::error_code result_ec) {
            called = true;
            handshake_ec = result_ec;
        });

    ioc.run();

    expect_true(called, "async failing handshake called");
    expect_ec_err(handshake_ec, "async handshake against plain server");

    server.join();

    std::cout << "  ok\n";
}

void test_async_read_after_peer_close()
{
    std::cout << "[test_async_read_after_peer_close]\n";

    lcy::asio::ssl::context server_ctx(lcy::asio::ssl::context::tls_server);
    configure_server_context(server_ctx);

    lcy::asio::ssl::context client_ctx(lcy::asio::ssl::context::tls_client);

    sync_ssl_server server(server_ctx,
        [](ssl_socket_type& socket) {
            lcy::system::error_code ec;
            socket.close(ec);
            expect_ec_ok(ec, "server close_notify");
        });
    server.start();

    lcy::asio::io_context ioc;
    ssl_socket_type client(ioc, client_ctx);

    lcy::system::error_code ec;
    connect_sync(client, server.endpoint(), ec);
    expect_ec_ok(ec, "client connect");

    bool handshake_done = false;
    bool read_called = false;
    lcy::system::error_code read_ec;
    char read_buffer[16] = { 0 };

    client.async_client_handshake(
        [&](lcy::system::error_code handshake_ec) {
            expect_ec_ok(handshake_ec, "async close handshake");
            handshake_done = true;

            client.async_read_some(
                lcy::asio::buffer_view(read_buffer, sizeof(read_buffer)),
                [&](lcy::system::error_code result_ec, std::size_t) {
                    read_called = true;
                    read_ec = result_ec;
                });
        });

    ioc.run();

    expect_true(handshake_done, "async close handshake done");
    expect_true(read_called, "async read after close called");
    expect_ec_err(read_ec, "async read after peer close_notify");

    server.join();

    std::cout << "  ok\n";
}

void test_mutual_tls_ok()
{
    std::cout << "[test_mutual_tls_ok]\n";

    lcy::asio::ssl::context server_ctx(lcy::asio::ssl::context::tls_server);
    configure_server_context(server_ctx);
    server_ctx.set_verify_mode(static_cast<lcy::asio::ssl::verify_mode>(
        lcy::asio::ssl::verify_peer |
        lcy::asio::ssl::verify_fail_if_no_peer_cert));

    lcy::system::error_code ec;
    server_ctx.load_verify_file(g_cert_path, ec);
    expect_ec_ok(ec, "server load client ca");

    lcy::asio::ssl::context client_ctx(lcy::asio::ssl::context::tls_client);
    configure_server_context(client_ctx);
    configure_client_verify(client_ctx);

    sync_ssl_server server(server_ctx,
        [](ssl_socket_type&) {});
    server.start();

    lcy::asio::io_context ioc;
    ssl_socket_type client(ioc, client_ctx);

    client.set_hostname("localhost", ec);
    expect_ec_ok(ec, "client set_hostname");
    client.set_verify_hostname("localhost", ec);
    expect_ec_ok(ec, "client set_verify_hostname");

    connect_sync(client, server.endpoint(), ec);
    expect_ec_ok(ec, "client connect");

    client.client_handshake(ec);
    expect_ec_ok(ec, "client mutual tls handshake");

    client.close(ec);
    expect_ec_ok(ec, "client close");

    server.join();
    expect_ec_ok(server.handshake_ec(), "server mutual tls handshake");
    expect_true(server.handshake_done(), "server mutual tls handshake done");

    std::cout << "  ok\n";
}

void test_mutual_tls_missing_client_cert()
{
    std::cout << "[test_mutual_tls_missing_client_cert]\n";

    lcy::asio::ssl::context server_ctx(lcy::asio::ssl::context::tls_server);
    configure_server_context(server_ctx);
    server_ctx.set_verify_mode(static_cast<lcy::asio::ssl::verify_mode>(
        lcy::asio::ssl::verify_peer |
        lcy::asio::ssl::verify_fail_if_no_peer_cert));

    lcy::system::error_code ec;
    server_ctx.load_verify_file(g_cert_path, ec);
    expect_ec_ok(ec, "server load client ca");

    lcy::asio::ssl::context client_ctx(lcy::asio::ssl::context::tls_client);

    sync_ssl_server server(server_ctx,
        [](ssl_socket_type&) {});
    server.start();

    lcy::asio::io_context ioc;
    ssl_socket_type client(ioc, client_ctx);

    connect_sync(client, server.endpoint(), ec);
    expect_ec_ok(ec, "client connect");

    client.client_handshake(ec);

    lcy::system::error_code ignored_ec;
    client.close(ignored_ec);

    server.join();
    expect_ec_err(server.handshake_ec(),
        "server handshake without client certificate");
    expect_true(!server.handshake_done(),
        "server mutual tls handshake not done");

    std::cout << "  ok\n";
}

void test_async_io_on_closed_socket()
{
    std::cout << "[test_async_io_on_closed_socket]\n";

    lcy::asio::io_context ioc;
    lcy::asio::ssl::context ctx(lcy::asio::ssl::context::tls_client);
    ssl_socket_type socket(ioc, ctx);

    char buffer[8] = { 0 };

    bool read_called = false;
    bool write_called = false;
    bool handshake_called = false;
    lcy::system::error_code read_ec;
    lcy::system::error_code write_ec;
    lcy::system::error_code handshake_ec;

    socket.async_read_some(
        lcy::asio::buffer_view(buffer, sizeof(buffer)),
        [&](lcy::system::error_code ec, std::size_t n) {
            expect_size(n, 0, "closed socket async read bytes");
            read_called = true;
            read_ec = ec;
        });

    socket.async_write_some(
        lcy::asio::buffer_view(buffer, sizeof(buffer)),
        [&](lcy::system::error_code ec, std::size_t n) {
            expect_size(n, 0, "closed socket async write bytes");
            write_called = true;
            write_ec = ec;
        });

    socket.async_client_handshake(
        [&](lcy::system::error_code ec) {
            handshake_called = true;
            handshake_ec = ec;
        });

    ioc.run();

    expect_true(read_called, "closed socket async read called");
    expect_true(write_called, "closed socket async write called");
    expect_true(handshake_called, "closed socket async handshake called");
    expect_ec_err(read_ec, "closed socket async read error");
    expect_ec_err(write_ec, "closed socket async write error");
    expect_ec_err(handshake_ec, "closed socket async handshake error");

    std::cout << "  ok\n";
}

void test_close_flushes_pending_write()
{
    std::cout << "[test_close_flushes_pending_write]\n";

    const std::size_t payload_size = 4096;

    lcy::asio::ssl::context server_ctx(lcy::asio::ssl::context::tls_server);
    configure_server_context(server_ctx);

    lcy::asio::ssl::context client_ctx(lcy::asio::ssl::context::tls_client);

    sync_ssl_server server(server_ctx,
        [payload_size](ssl_socket_type& socket) {
            std::vector<char> received(payload_size);
            lcy::system::error_code ec;

            std::size_t n = lcy::asio::read(socket,
                lcy::asio::buffer_view(received.data(), received.size()), ec);
            if ( ec ) {
                return;
            }
            expect_size(n, payload_size, "server read pending write payload");
            expect_pattern(received, 41, "server pending write payload pattern");

            char byte = 0;
            n = socket.read_some(lcy::asio::buffer_view(&byte, 1), ec);
            expect_ec_err(ec, "server read after client close");
            expect_size(n, 0, "server read after client close bytes");
        });
    server.start();

    lcy::asio::io_context ioc;
    ssl_socket_type client(ioc, client_ctx);

    lcy::system::error_code ec;
    connect_sync(client, server.endpoint(), ec);
    expect_ec_ok(ec, "client connect");

    client.client_handshake(ec);
    expect_ec_ok(ec, "client handshake");

    std::vector<char> payload(payload_size);
    fill_pattern(payload, 41);

    std::size_t n = client.write_some(
        lcy::asio::buffer_view(payload.data(), payload.size()), ec);
    expect_ec_ok(ec, "client write pending payload");
    expect_size(n, payload_size, "client write pending payload size");

    client.close(ec);
    expect_ec_ok(ec, "client close after write");

    server.join();
    expect_ec_ok(server.handshake_ec(), "server handshake");

    std::cout << "  ok\n";
}

void test_zero_length_io()
{
    std::cout << "[test_zero_length_io]\n";

    lcy::asio::ssl::context server_ctx(lcy::asio::ssl::context::tls_server);
    configure_server_context(server_ctx);

    lcy::asio::ssl::context client_ctx(lcy::asio::ssl::context::tls_client);

    sync_ssl_server server(server_ctx,
        [](ssl_socket_type& socket) {
            lcy::system::error_code ec;
            char byte = 0;

            std::size_t n = socket.read_some(
                lcy::asio::buffer_view(&byte, 0), ec);
            expect_ec_ok(ec, "server zero length read");
            expect_size(n, 0, "server zero length read bytes");

            n = socket.write_some(
                lcy::asio::buffer_view(&byte, 0), ec);
            expect_ec_ok(ec, "server zero length write");
            expect_size(n, 0, "server zero length write bytes");

            server_read_until_error(socket);
        });
    server.start();

    lcy::asio::io_context ioc;
    ssl_socket_type client(ioc, client_ctx);

    lcy::system::error_code ec;
    connect_sync(client, server.endpoint(), ec);
    expect_ec_ok(ec, "client connect");

    client.client_handshake(ec);
    expect_ec_ok(ec, "client handshake");

    char byte = 0;
    std::size_t n = client.read_some(
        lcy::asio::buffer_view(&byte, 0), ec);
    expect_ec_ok(ec, "client zero length read");
    expect_size(n, 0, "client zero length read bytes");

    n = client.write_some(
        lcy::asio::buffer_view(&byte, 0), ec);
    expect_ec_ok(ec, "client zero length write");
    expect_size(n, 0, "client zero length write bytes");

    bool async_read_called = false;
    bool async_write_called = false;
    lcy::system::error_code async_read_ec;
    lcy::system::error_code async_write_ec;
    std::size_t async_read_n = 1;
    std::size_t async_write_n = 1;

    client.async_read_some(
        lcy::asio::buffer_view(&byte, 0),
        [&](lcy::system::error_code result_ec, std::size_t result_n) {
            async_read_called = true;
            async_read_ec = result_ec;
            async_read_n = result_n;
        });

    client.async_write_some(
        lcy::asio::buffer_view(&byte, 0),
        [&](lcy::system::error_code result_ec, std::size_t result_n) {
            async_write_called = true;
            async_write_ec = result_ec;
            async_write_n = result_n;
        });

    ioc.run();

    expect_true(async_read_called, "async zero length read called");
    expect_true(async_write_called, "async zero length write called");
    expect_ec_ok(async_read_ec, "async zero length read");
    expect_ec_ok(async_write_ec, "async zero length write");
    expect_size(async_read_n, 0, "async zero length read bytes");
    expect_size(async_write_n, 0, "async zero length write bytes");

    lcy::system::error_code ignored_ec;
    client.close(ignored_ec);
    server.join();

    std::cout << "  ok\n";
}

}   // namespace

int main()
{
    setup_cert_files();

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

    test_local_handshake_sync();
    test_local_handshake_verify_peer_ok();
    test_local_handshake_verify_peer_no_ca();
    test_local_handshake_hostname_mismatch();
    test_server_handshake_without_cert();
    test_client_handshake_against_plain_server();
    test_server_handshake_against_plain_client();
    test_handshake_version_mismatch();
    test_handshake_twice_local();
    test_handshake_after_close_local();
    test_write_after_close_local();
    test_close_twice_local();
    test_write_read_large_sync();
    test_chunk_size_boundaries();
    test_read_short_buffer();
    test_peer_close_notify();
    test_peer_abrupt_close();
    test_get_strand_post();
    test_async_client_handshake_local();
    test_async_server_handshake_local();
    test_async_full_duplex_multithread();
    test_async_multiple_writes_via_strand();
    test_async_handshake_failure_plain_server();
    test_async_read_after_peer_close();
    test_mutual_tls_ok();
    test_mutual_tls_missing_client_cert();
    test_async_io_on_closed_socket();
    test_close_flushes_pending_write();
    test_zero_length_io();

    cleanup_cert_files();

    std::cout << "\nall ssl_stream_socket tests passed\n";
    return 0;
}