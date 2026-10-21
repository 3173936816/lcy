#include <array>
#include <memory>
#include <string>
#include <iostream>

#include "lcy/asio/ip/udp.hpp"
#include "lcy/asio/signal_set.hpp"
#include "lcy/asio/buffer_view.hpp"

class udp_connection :
	public std::enable_shared_from_this<udp_connection>
{
public:
    typedef std::shared_ptr<udp_connection> sptr;

	typedef lcy::asio::ip::udp::socket::connect_func_type connect_func_type;
	typedef lcy::asio::ip::udp::socket::read_func_type read_func_type;
	typedef lcy::asio::ip::udp::socket::write_func_type write_func_type;

    static sptr create_server(lcy::asio::io_context& ioc,
		lcy::asio::ip::udp::endpoint local_endpoint)
	{
		sptr conn(new udp_connection(ioc));

		lcy::system::error_code ec;
		conn->socket_.open(local_endpoint.protocol(), ec);
		if ( ec ) {
			std::cout << "create_server open : " << ec.message() << std::endl;
			return 0;
		}
		
		conn->socket_.bind(local_endpoint, ec);
		if ( ec ) {
			std::cout << "create_server bind : " << ec.message() << std::endl;
			return 0;
		}
		
		return conn;
	}

    static sptr create_client(lcy::asio::io_context& ioc,
		lcy::asio::ip::udp udp_protocol)
	{
		sptr conn(new udp_connection(ioc));		

		lcy::system::error_code ec;
		conn->socket_.open(udp_protocol, ec);
		if ( ec ) {
			std::cout << "create_server open : " << ec.message() << std::endl;
			return 0;
		}
		
		return conn;
	}

 	static sptr create_connect_client(lcy::asio::io_context& ioc,
		lcy::asio::ip::udp::endpoint remote_endpoint)
	{
		sptr conn(new udp_connection(ioc));		

		lcy::system::error_code ec;
		conn->socket_.open(remote_endpoint.protocol(), ec);
		if ( ec ) {
			std::cout << "create_server open : " << ec.message() << std::endl;
			return 0;
		}
		
		conn->socket_.connect(remote_endpoint, ec);
		if ( ec ) {
			std::cout << "create_server connect : " << ec.message() << std::endl;
			return 0;
		}
		
		return conn;
	}

	void do_receive_from_read()
	{
		auto self = shared_from_this();
		auto remote_endpoint_sptr = std::make_shared<lcy::asio::ip::udp::endpoint>();

		socket_.async_receive_from(*remote_endpoint_sptr,
			lcy::asio::buffer_view(read_buffer_),
				[remote_endpoint_sptr, this, self](lcy::system::error_code ec, std::size_t nbytes){
			if ( ec ) {
				std::cout << "do_receive_from_read error : " << ec.message() << std::endl;
				return;
			}

			std::cout << "do_receive_from_read -- remote_endpoint : "
				<< remote_endpoint_sptr->to_string() << ", data : "
				<< std::string(read_buffer_.data(), nbytes) << ", length : "
				<< nbytes << std::endl;
			
			::memcpy(write_buffer_.data(), read_buffer_.data(), nbytes);
			do_send_to_write(*remote_endpoint_sptr,
				lcy::asio::buffer_view(write_buffer_.data(), nbytes));
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

    lcy::asio::ip::udp::socket& socket()
	{ 
		return socket_;
	}

private:
	void do_send_to_write(lcy::asio::ip::udp::endpoint remote_endpoint,
		lcy::asio::const_buffer_view buffer_view)
	{
		auto self = shared_from_this();

		socket_.async_send_to(remote_endpoint, buffer_view,
				[this, self](lcy::system::error_code ec, std::size_t nbytes){
			if ( ec ) {
				std::cout << "do_send_to_write error : " << ec.message() << std::endl;
				return;
			}

			std::cout << "do_send_to_write -- length : "
				<< nbytes << std::endl;

			do_receive_from_read();
		});
	}

private:
    udp_connection(lcy::asio::io_context& ioc) :
		socket_(ioc) {}

	std::array<char, 2048> read_buffer_;
	std::array<char, 2048> write_buffer_;
    lcy::asio::ip::udp::socket socket_;
};

void test_receive_from_server()
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

	lcy::asio::ip::udp::endpoint local_endpoint(lcy::asio::ip::udp::v4(), 9950);
	auto conn = udp_connection::create_server(ioc, local_endpoint);

	conn->do_receive_from_read();

	ioc.run();
}

int main()
{
	test_receive_from_server();

	return 0;
}
