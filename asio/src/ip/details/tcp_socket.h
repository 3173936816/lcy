#ifndef __LCY_ASIO_IP_DETAILS_TCP_SOCKET_H__
#define __LCY_ASIO_IP_DETAILS_TCP_SOCKET_H__

#include <string>
#include <functional>

#include "lcy/asio/src/buffer.h"
#include "lcy/asio/src/errinfo.h"
#include "lcy/asio/src/io_context.hpp" 
#include "lcy/asio/src/ip/endpoint.h"

namespace lcy {
namespace asio {
namespace ip {
	class TCP;
namespace details {

class TCPSocket {
public:
	typedef std::function<void (errcode_type, size_t)> read_op_type;
	typedef std::function<void (errcode_type, size_t)> write_op_type;
	typedef std::function<void (errcode_type)> connect_op_type;
	typedef std::function<void (errcode_type)> accept_op_type;

	TCPSocket(IOContext& ioc);
	~TCPSocket();

	void async_read(MutableBuffer mbuf, read_op_type read_op);
	void async_write(ConstBuffer cbuf, write_op_type write_op);
	void async_accept(TCPSocket& tcp_socket, accept_op_type accept_op);
	void async_connect(const Endpoint& endpoint, connect_op_type connect_op);

	void cancel();

	errcode_type open(const TCP& tcp);
	errcode_type bind(const Endpoint& endpoint);
	errcode_type listen(int backlog);

	errcode_type setDelay();
	errcode_type setReuseAddr();
	errcode_type setKeepAlive();

	errcode_type shutdown();
	errcode_type shutdownRead();
	errcode_type shutdownWrite();

	errcode_type peerAddr(Endpoint& endpoint);
	errcode_type localAddr(Endpoint& endpoint);
	std::string tcpInfo();

	IOContext& context();

private:
	TCPSocket(const TCPSocket&);
	TCPSocket& operator=(const TCPSocket&);

private:
	typedef int sockfd_type;

	IOContext& ioc_;
	sockfd_type sockfd_;
	asio::details::ReactorService& reactor_;
};

}	// namespace details
}	// namespace ip
}	// namespace asio
}	// namespace lcy

#endif // __LCY_ASIO_IP_DETAILS_TCP_SOCKET_H__
