#include "http_connection.h"

#include <iostream>

HttpConnection::HttpConnection(lcy::asio::IOContext& ioc) :
	socket_(ioc)
{
}

HttpConnection::~HttpConnection()
{
}

void HttpConnection::setHttpOperation(http_op_type http_op)
{
	http_op_ = std::move(http_op);
}

void HttpConnection::start()
{
	start_read();
}

void HttpConnection::start_read()
{
	using namespace lcy::protocol::http;

	buffer_.reserve(4096 * 2);
	auto buf = lcy::asio::buffer(buffer_.writeBegin(),
								 buffer_.availableBytes());
	
	auto self = shared_from_this();
	socket_.async_read(buf, 
			[this, self](lcy::asio::errcode_type ec, size_t nread){
		if ( !ec ) {
			if ( nread == 0 ) {
				socket_.shutdown();
				return;
			}
			
			buffer_.write(nread);
			// std::cout << std::string(buffer_.readBegin(), buffer_.dataBytes()) << std::endl;
			
			// parse http request
			auto retcode = parser_.parse(buffer_.readBegin(),
										buffer_.dataBytes(),
										request_);
			if ( retcode == Parser::RetCode::ERROR ) {
			//	std::cout << "error" << std::endl;

				// error request
				std::string bad_request = "HTTP/1.1 400 Bad Request\r\n\r\n";
				start_send(std::move(bad_request), true);
				return;

			} else if ( retcode == Parser::RetCode::WAITING_DATA ) {
				// do nothing
			//	std::cout << "waiting" << std::endl;

			} else {
			//	std::cout << "ready" << std::endl;

				// parse ready
				Response response;
				http_op_(request_, response);

				if ( request_.version() == version::HTTP_1_0 ) {
					start_send(response.dump(), true);	// http1.0 is short link
					return;
				} else {
					start_send(response.dump(), false);
				}
				
				buffer_.read(parser_.nparse());
				request_.clear();
				parser_.reset();
			}
			
			// read again
			start_read();

		} else {
			if ( ec != lcy::asio::err::EOPCANCELED ) {
				std::cout << "socket async_read : "
						  << lcy::asio::errinfo(ec)
						  << std::endl;
				socket_.shutdown();
			}
		}
	});
}

void HttpConnection::start_send(std::string msg, bool shutdown)
{
	bool empty = response_op_deque_.empty();
	response_op_deque_.push_back({std::move(msg), shutdown});

	if ( empty ) {
		start_send_impl();
	}
}

void HttpConnection::start_send_impl()
{
	response_op_type& top_response_op = response_op_deque_.front();
	auto buf = lcy::asio::buffer(top_response_op.first.c_str(),
								 top_response_op.first.length());
	bool op = top_response_op.second;

	auto self = shared_from_this();
	socket_.async_write(buf,
			[this, self, op](lcy::asio::errcode_type ec, size_t nwrite){
		if ( !ec ) {
			response_op_deque_.pop_front();
			if ( op ) {	// need shutdown
				socket_.shutdown();
				return;
			}

			if ( !response_op_deque_.empty() ) {
				// write again
				start_send_impl();
			}
			
		} else {
			if ( ec != lcy::asio::err::EOPCANCELED ) {
				std::cout << "socket async_write : "
						  << lcy::asio::errinfo(ec)
						  << std::endl;
				socket_.shutdown();
			}
		}
	});
}

lcy::asio::ip::TCP::Socket& HttpConnection::get_socket()
{
	return socket_;
}
