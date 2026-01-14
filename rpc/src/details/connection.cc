#include "lcy/rpc/src/details/connection.h"

#include <string.h>
#include <arpa/inet.h>

namespace lcy {
namespace rpc {
namespace details {

Connection::Connection(lcy::asio::IOContext& ioc) :
	socket_(ioc)
{
}

Connection::~Connection()
{
	shutdown();
}

void Connection::shutdown()
{
	socket_.shutdown();
}

void Connection::start_recv()
{
	read_buf_.reserve(4096);	
	auto cbuf = lcy::asio::buffer(read_buf_.writeBegin(), read_buf_.availableBytes());

	auto self = shared_from_this();
	socket_.async_read(cbuf, std::bind(&Connection::onRecvMessage,
			this, self, std::placeholders::_1, std::placeholders::_2));
}

void Connection::send(std::string data)
{
	bool need_start_deque_send = send_buf_deque_.empty();
	send_buf_deque_.push_back(std::move(data));

	if ( need_start_deque_send ) {
		start_deque_send();
	}
}

void Connection::connect(const std::string& ip, uint16_t port)
{
	lcy::asio::ip::Endpoint endpoint(ip, port);
	if ( endpoint.isV4() ) {
		socket_.open(lcy::asio::ip::TCP::v4());
	}
	if ( endpoint.isV6() ) {
		socket_.open(lcy::asio::ip::TCP::v6());
	}

	auto self = shared_from_this();
	socket_.async_connect(endpoint, std::bind(&Connection::onConnect,
			this, self, std::placeholders::_1));
}

void Connection::setConnectOp(connect_op_type connect_op)
{
	connect_op_ = std::move(connect_op);
}

void Connection::setMessageOp(message_op_type message_op)
{
	message_op_ = std::move(message_op);
}

lcy::asio::IOContext& Connection::context()
{
	return socket_.context();
}

lcy::asio::ip::TCP::Socket& Connection::get_socket()
{
	return socket_;
}

void Connection::onRecvMessage(ConnPtr conn, lcy::asio::errcode_type ec, size_t nbytes)
{
	if ( !ec ) {
		if ( nbytes == 0 ) {
			conn->shutdown();
			return;
		}

		read_buf_.write(nbytes);

		while ( true ) {
			if ( read_buf_.dataBytes() < sizeof(uint32_t) ) {
				break;
			}

			uint32_t len = 0;
			::memcpy(&len, read_buf_.readBegin(), sizeof(uint32_t));
			len = ntohl(len);

			if ( read_buf_.dataBytes() < len + sizeof(uint32_t) ) {
				break;
			}

			std::string data = std::string(read_buf_.readBegin() + sizeof(uint32_t), len);
			read_buf_.read(len + sizeof(uint32_t));

			if ( message_op_ ) {
				message_op_(*this, data);
			}
		}

		start_recv();
	}
}

void Connection::start_deque_send()
{
	const std::string& data = send_buf_deque_.front();
	auto cbuf = lcy::asio::buffer(data);

	auto self = shared_from_this();
	socket_.async_write(cbuf, std::bind(&Connection::onSendCompleted,
			this, self, std::placeholders::_1, std::placeholders::_2));
}

void Connection::onSendCompleted(ConnPtr conn, lcy::asio::errcode_type ec, size_t nbytes)
{
	send_buf_deque_.pop_front();
	
	if ( !send_buf_deque_.empty() ) {
		start_deque_send();
	}
}

void Connection::onConnect(ConnPtr conn, lcy::asio::errcode_type ec)
{
	if ( connect_op_ ) {
		connect_op_(ec);
	}
}

}	// namespace details
}	// namespace rpc
}	// namespace lcy
