#ifndef __LCY_ASIO_IP_DETAILS_UDP_SOCKET_H__
#define __LCY_ASIO_IP_DETAILS_UDP_SOCKET_H__

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>

#include "asio/src/buffer.h"
#include "asio/src/io_context.hpp" 
#include "asio/src/details/channel.h" 
#include "asio/src/ip/udp.h"
#include "asio/src/ip/endpoint.h"

namespace lcy {
namespace asio {
namespace ip {
	class UDP;
namespace details {

class UDPSocket {
public:
	typedef std::function<void (int, size_t)> read_callback_type;
	typedef std::function<void (int, size_t)> write_callback_type;

	UDPSocket(IOContext& ioc);
	~UDPSocket();

	void async_read(EndPoint& endpoint, 
					MutableBuffer mbuf, 
					read_callback_type read_cb);
	void async_write(const EndPoint& endpoint, 
					 ConstBuffer cbuf, 
					 write_callback_type write_cb);
	void cancel();	

	int open(UDP udp);
	int bind(const EndPoint& endpoint);

	int setReuseAddr();

	int shutdown();
	int shutdownRead();
	int shutdownWrite();

	std::string udpInfo();

private:
	UDPSocket(const UDPSocket&);
	UDPSocket& operator=(const UDPSocket&);

private:
	asio::details::Channel channel_;
};

}	// namespace details
}	// namespace ip
}	// namespace asio
}	// namespace lcy

#endif // __LCY_ASIO_IP_DETAILS_UDP_SOCKET_H__
