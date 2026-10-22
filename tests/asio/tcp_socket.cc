#include <array>
#include <memory>
#include <string>
#include <iostream>

#include "lcy/asio/ip/tcp.hpp"
#include "lcy/asio/signal_set.hpp"
#include "lcy/asio/buffer_view.hpp"

class tcp_connection :
    public std::enable_shared_from_this<tcp_connection>
{
public:
    typedef std::shared_ptr<tcp_connection> sptr;

    typedef lcy::asio::ip::tcp::socket::connect_func_type connect_func_type;
	typedef lcy::asio::ip::tcp::socket::read_func_type read_func_type;
	typedef lcy::asio::ip::tcp::socket::write_func_type write_func_type;

public:
    tcp_connection(lcy::asio::io_context& ioc) :
		socket_(ioc) {}

    void do_read()
    {
        auto self = shared_from_this();

        socket_.async_read_some(lcy::asio::buffer_view(read_buffer_),
				[this, self](lcy::system::error_code ec, std::size_t nbytes){
			if ( ec ) {
				std::cout << "do_read error : " << ec.message() << std::endl;
				return;
			}

			std::cout << "do_read -- data : "
				<< std::string(read_buffer_.data(), nbytes) << ", length : "
				<< nbytes << std::endl;

			::memcpy(write_buffer_.data(), read_buffer_.data(), nbytes);
			do_write(lcy::asio::buffer_view(write_buffer_.data(), nbytes));
		});
    }

    void do_write(lcy::asio::const_buffer_view buffer_view)
    {
        auto self = shared_from_this();

		socket_.async_write_some(buffer_view,
				[this, self](lcy::system::error_code ec, std::size_t nbytes){
			if ( ec ) {
				std::cout << "do_write error : " << ec.message() << std::endl;
				return;
			}

			std::cout << "do_write -- length : "
				<< nbytes << std::endl;

			do_read();
		});
    }

    void cancel()
	{
		socket_.cancel();
	}

    void close()
	{
		socket_.close();
	}

    lcy::asio::ip::tcp::socket& socket()
	{ 
		return socket_;
	}

private:
	std::array<char, 2048> read_buffer_;
	std::array<char, 2048> write_buffer_;
    lcy::asio::ip::tcp::socket socket_;
};

class tcp_server
{
public:
    tcp_server(lcy::asio::io_context& ioc,
               lcy::asio::ip::tcp::endpoint local_endpoint) :
        ioc_(ioc), acceptor_(ioc, local_endpoint) {}

    void do_accept()
    {
        auto conn = std::make_shared<tcp_connection>(ioc_);
        auto remote_endpoint = std::make_shared<lcy::asio::ip::tcp::endpoint>();

        acceptor_.async_accept(conn->socket(), *remote_endpoint,
                [conn, remote_endpoint, this](lcy::system::error_code ec){
            if ( ec ) {
                std::cout << "accept error : " << ec.message() << std::endl;
                return;
            }

            std::cout << "accept -- remote_endpoint :"
                << remote_endpoint->to_string() << std::endl;

            conn->do_read();
            do_accept();
        });
    }

private:
    lcy::asio::io_context& ioc_;
    lcy::asio::ip::tcp::acceptor acceptor_;
};

void test_server()
{
	lcy::asio::io_context ioc;

	lcy::asio::signal_set signal_set(ioc, { SIGINT });
	signal_set.async_wait([&ioc](lcy::system::error_code ec, int signum){
		if ( ec ) {
			std::cout << "signal_set error : " << ec.message() << std::endl;
			return;
		}

		ioc.stop();
		std::cout << "receive signal : " << signum << ", server exit" << std::endl;
	});

    lcy::asio::ip::tcp::endpoint local_endpoint(lcy::asio::ip::tcp::v4(), 9950);
    tcp_server server(ioc, local_endpoint);
    server.do_accept();

	std::vector<std::thread> threads(12);
	for ( auto& thread : threads ) {
		thread = std::thread([&ioc](){
			ioc.run();
		});
	}

	for ( auto& thread : threads ) {
		thread.join();
	}
}

int main()
{
    test_server();

    return 0;
}