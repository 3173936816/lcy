#ifndef __LCY_ASIO_IP_DETAILS_TCP_SOCKET_H__
#define __LCY_ASIO_IP_DETAILS_TCP_SOCKET_H__

#include <string>
#include <functional>

#include "asio/src/buffer.h"
#include "asio/src/io_context.hpp" 
#include "asio/src/details/channel.h" 
#include "asio/src/ip/endpoint.h"

namespace lcy {
namespace asio {
namespace ip {
	class TCP;
namespace details {

class TCPSocket {
public:
	typedef std::function<void (int, size_t)> read_callback_type;
	typedef std::function<void (int, size_t)> write_callback_type;
	typedef std::function<void (int)> connect_callback_type;
	typedef std::function<void (int)> accept_callback_type;

	TCPSocket(IOContext& ioc);
	~TCPSocket();

	void async_read(MutableBuffer mbuf, read_callback_type read_cb);
	void async_write(ConstBuffer cbuf, write_callback_type write_cb);
	void async_accept(TCPSocket& tcp_socket, accept_callback_type accept_cb);
	void async_connect(const EndPoint& endpoint, connect_callback_type connect_cb);

	void cancel();

	int open(const TCP& tcp);
	int bind(const EndPoint& endpoint);
	int listen(int backlog);

	int setDelay();
	int setReuseAddr();
	int setKeepAlive();

	int shutdown();
	int shutdownRead();
	int shutdownWrite();

	int peerAddr(EndPoint& endpoint);
	int localAddr(EndPoint& endpoint);
	std::string tcpInfo();

private:
	TCPSocket(const TCPSocket&);
	TCPSocket& operator=(const TCPSocket&);

private:
	asio::details::Channel channel_;
};

}	// namespace details
}	// namespace ip
}	// namespace asio
}	// namespace lcy

#endif // __LCY_ASIO_IP_DETAILS_TCP_SOCKET_H__
