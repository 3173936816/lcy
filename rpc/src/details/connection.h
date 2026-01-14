#ifndef __LCY_RPC_DETAILS_CONNECTION_H__
#define __LCY_RPC_DETAILS_CONNECTION_H__

#include <deque>
#include <memory>
#include <string>

#include "lcy/asio/asio.hpp"

namespace lcy {
namespace rpc {
namespace details {

class Connection :
	public std::enable_shared_from_this<Connection>
{
public:
	typedef std::function<void (lcy::asio::errcode_type)> connect_op_type;
	typedef std::function<void (Connection&, const std::string&)> message_op_type;

	Connection(lcy::asio::IOContext& ioc);
	~Connection();

	void shutdown();
	void start_recv();
	void send(std::string data);
	void connect(const std::string& ip, uint16_t port);

	void setConnectOp(connect_op_type connect_op);
	void setMessageOp(message_op_type message_op);

	lcy::asio::IOContext& context();
	lcy::asio::ip::TCP::Socket& get_socket();

private:
	typedef std::shared_ptr<Connection> ConnPtr;

	void onRecvMessage(ConnPtr conn, lcy::asio::errcode_type ec, size_t nbytes);
	void start_deque_send();
	void onSendCompleted(ConnPtr conn, lcy::asio::errcode_type ec, size_t nbytes);
	void onConnect(ConnPtr conn, lcy::asio::errcode_type ec);
	
private:
	lcy::asio::ip::TCP::Socket socket_;
	lcy::asio::DynamicBuffer read_buf_;
	std::deque<std::string> send_buf_deque_;

	message_op_type message_op_;
	connect_op_type connect_op_;
};

}	// namespace details
}	// namespace rpc
}	// namespace lcy

#endif // __LCY_RPC_DETAILS_CONNECTION_H__
