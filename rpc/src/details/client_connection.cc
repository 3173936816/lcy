#include "lcy/rpc/src/details/client_connection.h"

#include <string.h>

namespace lcy {
namespace rpc {
namespace details {

ClientConnection::ClientConnection(lcy::asio::IOContext& ioc) :
	timer_(ioc, reconnection_interval),
	curr_reconnection_count_(0),
	socket_(ioc)
{
}

ClientConnection::~ClientConnection()
{
	shutdown();
}

void ClientConnection::start_async_recv()
{
	read_buf_.reserve(4096);
	auto mbuf = lcy::asio::buffer(read_buf_.writeBegin(), read_buf_.availableBytes());
	socket_.async_read(mbuf,
			[this](lcy::asio::errcode_type ec, size_t nbytes){
		if ( message_op_ ) {
			message_op_(ec, nbytes, read_buf_);
		}
	});
}

void ClientConnection::start_async_connect(const lcy::asio::ip::Endpoint& endpoint)
{
	if ( endpoint.isV4() )
		socket_.open(lcy::asio::ip::TCP::v4());
	else 
		socket_.open(lcy::asio::ip::TCP::v6());

	peer_addr_ = endpoint;

	socket_.async_connect(endpoint,
			[this](lcy::asio::errcode_type ec){
		if ( !ec ) {
			curr_reconnection_count_ = 0;
			socket_.peerAddr(peer_addr_);
			if ( connect_op_ ) {
				connect_op_(ec);
			}
		} else {
			startConnectTimer();
		}
	});
}

void ClientConnection::start_async_send(lcy::asio::ConstBuffer cbuf)
{
	socket_.async_write(cbuf,
			[this](lcy::asio::errcode_type ec, size_t nbytes){
		send_completed_op_(ec, nbytes);
	});
}

void ClientConnection::shutdown()
{
	timer_.cancel();
	socket_.shutdown();
}

lcy::asio::ip::Endpoint ClientConnection::localAddr() const
{
	return local_addr_;
}

lcy::asio::ip::Endpoint ClientConnection::peerAddr() const
{
	return peer_addr_;
}

lcy::asio::IOContext& ClientConnection::context()
{
	return socket_.context();
}

void ClientConnection::setConnectOp(connect_op_type connect_op)
{
	connect_op_ = std::move(connect_op);
}

void ClientConnection::setMessageOp(message_op_type message_op)
{
	message_op_ = std::move(message_op);
}

void ClientConnection::setSendCompletedOp(send_completed_op_type send_completed_op)
{
	send_completed_op_ = std::move(send_completed_op);
}

void ClientConnection::startConnectTimer()
{
	timer_.async_wait(std::bind(&ClientConnection::onTimer,
		 this, std::placeholders::_1, std::placeholders::_2));
}

void ClientConnection::onTimer(lcy::asio::errcode_type ec, time_t timeout)
{
	if ( !ec ) {
		if ( ++curr_reconnection_count_ >= max_reconnection_count) {
			if ( connect_op_ ) {
				curr_reconnection_count_ = 0;
				connect_op_(ETIMEDOUT);
				return;
			}
		}
		this->start_async_connect(peer_addr_);
	}
}

}	// namespace details
}	// namespace rpc
}	// namespace lcy
