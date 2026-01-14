#ifndef __LCY_RPC_DETAILS_SERVER_H__
#define __LCY_RPC_DETAILS_SERVER_H__

#include <string>

#include "lcy/asio/asio.hpp"

namespace lcy {
namespace rpc {
namespace details {

class Connection;

class Server {
public:
	typedef std::function<void (Connection&)> connect_op_type;

	Server(lcy::asio::IOContext& ioc,
		   uint32_t threadCount);
	~Server();

	void start(const lcy::asio::ip::Endpoint& addr);
	void stop();
	void setConnectOp(connect_op_type connect_op);

private:
	void start_accept();

private:
	lcy::asio::ip::TCP::Acceptor acceptor_;
	lcy::asio::ThreadPool io_thread_pool_;

	connect_op_type connect_op_;
};

}	// namespace details
}	// namespace rpc
}	// namespace lcy

#endif // __LCY_RPC_DETAILS_SERVER_H__
