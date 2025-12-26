#ifndef __LCY_NET_UTIL_TCP_CONNECTION_H__
#define __LCY_NET_UTIL_TCP_CONNECTION_H__

#include <memory>

#include "lcy/asio/asio.hpp"
#include "lcy/net_util/src/operations.h"

namespace lcy {
namespace net_util {

class TCPConnection :
	public std::enable_shared_from_this<TCPConnection>
{
public:
	TCPConnection(lcy::asio::IOContext& ioc);
	~TCPConnection();

	void shutdown();

	void async_send(const std::string& data);
	void async_send(void* data, size_t len);
	void async_connect(const lcy::asio::ip::Endpoint& peer_addr);
	lcy::asio::ip::Endpoint localAddr() const;
	lcy::asio::ip::Endpoint peerAddr() const;

	std::shared_ptr<void> getUserData();
	void setUserData(std::shared_ptr<void> user_data);

	lcy::asio::ip::TCP::Socket& get_socket();

	void setNewConnOp(new_conn_op_type new_conn_op);
	void setMessageOp(message_op_type message_op);
	void setSendFinishedOp(send_finished_op_type send_finished_op);
	void setPeerCloseOp(peer_close_op_type peer_close_op);
	void setErrorOp(error_op_type error_op);

	new_conn_op_type getNewConnOp();
	message_op_type getMessageOp();
	send_finished_op_type getSendFinishedOp();
	peer_close_op_type getPeerCloseOp();
	error_op_type getErrorOp();

private:
	TCPConnection(const TCPConnection&);
	TCPConnection& operator=(const TCPConnection&);

private:
	lcy::asio::ip::TCP::Socket socket_;
	lcy::asio::DynamicBuffer read_buf_;
	lcy::asio::DynamicBuffer write_buf_;

	lcy::asio::ip::Endpoint local_addr_;
	lcy::asio::ip::Endpoint peer_addr_;
	std::shared_ptr<void> user_data_;

	new_conn_op_type new_conn_op_;
	message_op_type message_op_;
	send_finished_op_type send_finished_op_;
	peer_close_op_type peer_close_op_;
	error_op_type error_op_;
};

}	// namespace net_util
}	// namespace lcy

#endif // __LCY_NET_UTIL_TCP_CONNECTION_H__
