#include <memory>
#include <iostream>
#include <functional>

#include "lcy/asio/ip/tcp.hpp"
#include "lcy/asio/buffer_view.hpp"
#include "lcy/asio/async_stream.hpp"
#include "lcy/asio/ip/socket_option.hpp"

#include "lcy/bytra/flat_buffer.hpp"
#include "lcy/bytra/http/async_stream.hpp"

namespace {

class http_connection :
    public std::enable_shared_from_this<http_connection>
{
public:
    typedef std::function<
        void (
            const lcy::bytra::http::request& request,
            lcy::bytra::http::response& response
        )
    > func_type;

    http_connection(lcy::asio::io_context& ioc, func_type func) :
        socket_(ioc), func_(std::move(func)) {}

    void start()
    {
        parser_.reset();

        auto self = shared_from_this();
        auto request = std::make_shared<lcy::bytra::http::request>();

        lcy::bytra::http::async_read(socket_, read_buffer_, parser_, *request,
                [self, this, request](lcy::system::error_code ec, std::size_t){
            if ( ec ) {
                std::cout << "async_read ec = " << ec.message() << std::endl;
                return;
            }

            std::cout << "async_read ok" << std::endl;

            auto response = std::make_shared<lcy::bytra::http::response>();
            func_(*request, *response);

            lcy::bytra::http::async_write(socket_, *response,
                    [self, this, response](lcy::system::error_code ec, std::size_t){
                if ( ec ) {
                    std::cout << "async_write ec = " << ec.message() << std::endl;
                    return;
                }

                std::cout << "async_write ok" << std::endl;

                start();
            });
        });
    }

    lcy::asio::ip::tcp::socket& get_socket()
    {
        return socket_;
    }

private:
    lcy::asio::ip::tcp::socket socket_;
    lcy::bytra::flat_buffer read_buffer_;
    lcy::bytra::http::request_parser parser_;
    func_type func_;
};

class http_server
{
public:
    typedef http_connection::func_type func_type;

    http_server(lcy::asio::io_context& ioc,
                lcy::asio::ip::tcp::endpoint ep,
                func_type func)
        : ioc_(ioc), acceptor_(ioc, ep, lcy::asio::ip::socket_option::reuse_address(1)), func_(std::move(func)) {}

    void do_accept()
    {
        auto conn = std::make_shared<http_connection>(ioc_, func_);
        auto remote = std::make_shared<lcy::asio::ip::tcp::endpoint>();

        acceptor_.async_accept(conn->get_socket(), *remote,
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
    lcy::asio::ip::tcp::acceptor acceptor_;
    func_type func_;
};

}   // namespace

int main()
{
    lcy::asio::io_context ioc;
    lcy::asio::ip::tcp::endpoint ep(
        lcy::asio::ip::tcp::v4(), 9950);

    http_server server(ioc, ep,
        [](const lcy::bytra::http::request& request,
           lcy::bytra::http::response& response)
        {
            std::cout << "request header : " << request.header_to_string() << std::endl;

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