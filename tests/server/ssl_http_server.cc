#include <memory>
#include <iostream>
#include <functional>
#include <cstring>

#include "lcy/asio/ip/tcp.hpp"
#include "lcy/asio/ssl/ssl.hpp"
#include "lcy/asio/buffer_view.hpp"
#include "lcy/asio/async_stream.hpp"
#include "lcy/asio/ip/socket_option.hpp"

#include "lcy/bytra/flat_buffer.hpp"
#include "lcy/bytra/http/async_stream.hpp"

namespace {

typedef lcy::bytra::flat_buffer flat_buf;
typedef lcy::bytra::http::request<flat_buf> req_t;
typedef lcy::bytra::http::response<flat_buf> res_t;
typedef lcy::bytra::http::request_parser<flat_buf> req_parser_t;
typedef lcy::asio::ssl::stream_socket ssl_stream_socket_t;

class ssl_http_connection :
    public std::enable_shared_from_this<ssl_http_connection>
{
public:
    typedef std::function<
        void (
            const req_t& request,
            res_t& response
        )
    > func_type;

    ssl_http_connection(lcy::asio::io_context& ioc,
                        lcy::asio::ssl::context& ssl_ctx,
                        func_type func) :
        socket_(ioc, ssl_ctx), func_(std::move(func)) {}

    void start()
    {
        parser_.reset();
        read_buffer_.clear();

        auto self = shared_from_this();

        lcy::system::error_code ec;
        socket_.get_next_layer().set_option(
            lcy::asio::ip::socket_option::keep_alive(1), ec);
        if ( ec ) {
            std::cout << "set_option keep_alive ec = " << ec.message() << "\n";
        }

        socket_.async_server_handshake(
            [self, this](lcy::system::error_code ec) {
                if ( ec ) {
                    std::cout << "async_server_handshake ec = "
                              << ec.message() << "\n";
                    return;
                }

                std::cout << "ssl handshake ok\n";

                do_read_request();
            });
    }

    lcy::asio::ip::tcp::socket& get_next_layer()
    {
        return socket_.get_next_layer();
    }

private:
    void do_read_request()
    {
        auto self = shared_from_this();
        auto request = std::make_shared<req_t>();

        lcy::bytra::http::async_read(socket_, read_buffer_, parser_, *request,
            [self, this, request](lcy::system::error_code ec, std::size_t) {
                if ( ec ) {
                    std::cout << "async_read ec = " << ec.message() << "\n";
                    return;
                }

                std::cout << "async_read ok\n";

                auto response = std::make_shared<res_t>();
                func_(*request, *response);

                lcy::bytra::http::async_write(socket_, *response,
                    [self, this, response](lcy::system::error_code ec,
                                           std::size_t) {
                        if ( ec ) {
                            std::cout << "async_write ec = "
                                      << ec.message() << "\n";
                            return;
                        }

                        std::cout << "async_write ok\n";

                        do_read_request();
                    });
            });
    }

private:
    ssl_stream_socket_t socket_;
    flat_buf read_buffer_;
    req_parser_t parser_;
    func_type func_;
};

class ssl_http_server
{
public:
    typedef ssl_http_connection::func_type func_type;

    ssl_http_server(lcy::asio::io_context& ioc,
                    lcy::asio::ssl::context& ssl_ctx,
                    lcy::asio::ip::tcp::endpoint ep,
                    func_type func)
        : ioc_(ioc),
          ssl_ctx_(ssl_ctx),
          acceptor_(ioc, ep, lcy::asio::ip::socket_option::reuse_address(1)),
          func_(std::move(func)) {}

    void do_accept()
    {
        auto conn = std::make_shared<ssl_http_connection>(
            ioc_, ssl_ctx_, func_);
        auto remote = std::make_shared<lcy::asio::ip::tcp::endpoint>();

        acceptor_.async_accept(conn->get_next_layer(), *remote,
            [conn, remote, this](lcy::system::error_code ec) {
                if ( ec ) {
                    std::cout << "    [server] accept error: "
                              << ec.message() << "\n";
                    return;
                }
                std::cout << "    [server] accepted "
                          << remote->to_string() << "\n";
                conn->start();
                do_accept();
            });
    }

    lcy::asio::ip::tcp::endpoint local_endpoint()
    {
        lcy::asio::ip::tcp::endpoint ep;
        acceptor_.local_endpoint(ep);
        return ep;
    }

private:
    lcy::asio::io_context& ioc_;
    lcy::asio::ssl::context& ssl_ctx_;
    lcy::asio::ip::tcp::acceptor acceptor_;
    func_type func_;
};

}   // namespace

int main()
{
    lcy::asio::io_context ioc;

    lcy::asio::ssl::context ssl_ctx(
        lcy::asio::ssl::context::tls_server);

    lcy::system::error_code ec;
    ssl_ctx.use_certificate_file("cert.pem",
        lcy::asio::ssl::context::pem, ec);
    if ( ec ) {
        std::cerr << "use_certificate_file: " << ec.message() << "\n";
        return 1;
    }

    ssl_ctx.use_private_key_file("key.pem",
        lcy::asio::ssl::context::pem, ec);
    if ( ec ) {
        std::cerr << "use_private_key_file: " << ec.message() << "\n";
        return 1;
    }

    ssl_ctx.set_verify_mode(lcy::asio::ssl::verify_none);

    lcy::asio::ip::tcp::endpoint ep(
        lcy::asio::ip::tcp::v4(), 9950);

    ssl_http_server server(ioc, ssl_ctx, ep,
        [](const req_t& request, res_t& response)
        {
            std::cout << "request header : " << request.header_to_string()
                      << std::endl;

            response.set_status(lcy::bytra::http::status::ok);
            response.set_version(lcy::bytra::http::version::http_1_1);
            response.set_field("Content-Type", "text/plain; charset=utf-8");
            response.set_field("Content-Length", "5");

            auto view = response.body_container().prepare(5);
            std::memcpy(view.data(), "hello", 5);
            response.body_container().commit(5);
        });

    server.do_accept();
    ioc.run();

    return 0;
}