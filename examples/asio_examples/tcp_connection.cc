#include "tcp_connection.h"

#include <iostream>
#include <string.h>

TCPConnection::TCPConnection(lcy::asio::IOContext& ioc) :
	socket_(ioc)
{
	read_buffer_.reserve(4096);
	write_buffer_.reserve(4096);
}

TCPConnection::~TCPConnection()
{
}

void TCPConnection::start()
{
	start_read();
}

void TCPConnection::start_read()
{
	auto self = shared_from_this();
	auto buf = lcy::asio::buffer(read_buffer_.writeBegin(),
								 read_buffer_.availableBytes());

	socket_.async_read(buf, [this, self](lcy::asio::errcode_type ec, size_t nbytes){
		if ( !ec ) {
			if ( nbytes == 0 ) {
				shutdown();
				return;
			}

			read_buffer_.write(nbytes);

			processData();

			start_read();	// read again

		} else {
			std::clog << "socket async_read : " 
					  << lcy::asio::errinfo(ec) << std::endl;
		}
	});
}

void TCPConnection::shutdown()
{
	socket_.shutdown();
}

lcy::asio::ip::TCP::Socket& TCPConnection::get_socket()
{
	return socket_;
}

void TCPConnection::processData()
{
	auto self = shared_from_this();
	
	size_t nbytes = read_buffer_.dataBytes();
	::memcpy(write_buffer_.writeBegin(), read_buffer_.readBegin(), nbytes);
	read_buffer_.read(nbytes);
	write_buffer_.write(nbytes);

	auto buf = lcy::asio::buffer(write_buffer_.readBegin(), write_buffer_.dataBytes());

	socket_.async_write(buf, [this, self](lcy::asio::errcode_type ec, size_t nbytes){
		if ( !ec ) {
			write_buffer_.read(nbytes);
		} else {
			std::clog << "socket async_write : " 
					  << lcy::asio::errinfo(ec) << std::endl;
		}
	});
}
