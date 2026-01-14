#ifndef __LCY_ASIO_IP_DETAILS_UDP_SOCKET_H__
#define __LCY_ASIO_IP_DETAILS_UDP_SOCKET_H__

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>

#include "lcy/asio/src/errinfo.h"
#include "lcy/asio/src/buffer.h"
#include "lcy/asio/src/io_context.hpp" 
#include "lcy/asio/src/ip/endpoint.h"

namespace lcy {
namespace asio {
namespace ip {
	class UDP;
namespace details {

class UDPSocket {
public:
	typedef std::function<void (int, size_t)> read_op_type;
	typedef std::function<void (int, size_t)> write_op_type;

	UDPSocket(IOContext& ioc);
	~UDPSocket();

	void async_read(Endpoint& endpoint, 
					MutableBuffer mbuf, 
					read_op_type read_op);
	void async_write(const Endpoint& endpoint,
					 ConstBuffer cbuf, 
					 write_op_type write_op);
	void cancel();

	errcode_type open(const UDP& udp);
	errcode_type bind(const Endpoint& endpoint);

	errcode_type setReuseAddr();

	errcode_type shutdown();
	errcode_type shutdownRead();
	errcode_type shutdownWrite();

	std::string udpInfo();

private:
	UDPSocket(const UDPSocket&);
	UDPSocket& operator=(const UDPSocket&);

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

#endif // __LCY_ASIO_IP_DETAILS_UDP_SOCKET_H__
