#ifndef __LCY_RPC_DETAILS_CLIENT_CONNECTION_H__
#define __LCY_RPC_DETAILS_CLIENT_CONNECTION_H__

#include <memory>
#include <string>
#include <functional>

#include "lcy/asio/asio.hpp"

namespace lcy {
namespace rpc {
namespace details {

class ClientConnection {
public:
	typedef std::function<void (
				lcy::asio::errcode_type
			)> connect_op_type;

	typedef std::function<void (
				lcy::asio::errcode_type,
				size_t nbytes,
				lcy::asio::DynamicBuffer&)
			> message_op_type;

	typedef std::function<void (
				lcy::asio::errcode_type, size_t
			)> send_completed_op_type;

	ClientConnection(lcy::asio::IOContext& ioc);
	~ClientConnection();

	void start_async_recv();
	void start_async_connect(const lcy::asio::ip::Endpoint& endpoint);
	void start_async_send(lcy::asio::ConstBuffer cbuf);

	void shutdown();
	lcy::asio::ip::Endpoint localAddr() const;
	lcy::asio::ip::Endpoint peerAddr() const;

	lcy::asio::IOContext& context();

	void setConnectOp(connect_op_type connect_op);
	void setMessageOp(message_op_type message_op);
	void setSendCompletedOp(send_completed_op_type send_completed_op);

private:
	void startConnectTimer();
	void onTimer(lcy::asio::errcode_type ec, time_t timeout);

	enum {
		max_reconnection_count = 5,
		reconnection_interval = 5000,
	};

private:
	lcy::asio::SteadyTimer timer_;
	uint32_t curr_reconnection_count_;
	lcy::asio::ip::TCP::Socket socket_;
	lcy::asio::DynamicBuffer read_buf_;

	lcy::asio::ip::Endpoint local_addr_;
	lcy::asio::ip::Endpoint peer_addr_;

	connect_op_type connect_op_;
	message_op_type message_op_;
	send_completed_op_type send_completed_op_;
};

}	// namespace details
}	// namespace rpc
}	// namespace lcy

#endif // __LCY_RPC_DETAILS_CLIENT_CONNECTION_H__
