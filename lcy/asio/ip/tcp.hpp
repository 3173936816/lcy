#ifndef __LCY_ASIO_IP_TCP_HPP__
#define __LCY_ASIO_IP_TCP_HPP__

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include "lcy/asio/io_context.hpp"
#include "lcy/asio/detail/basic_endpoint.hpp"
#include "lcy/asio/detail/basic_stream_socket.hpp"

namespace lcy
{
namespace asio
{
namespace ip
{

class tcp
{
private:
	tcp(int family) :
		family_(family) {}

public:
	typedef detail::basic_endpoint<tcp> endpoint;

	typedef detail::basic_stream_socket<
		io_context, tcp, endpoint> socket;

	static tcp v4()
	{
		return tcp(AF_INET);
	}

	static tcp v6()
	{
		return tcp(AF_INET6);
	}

	int family() const
	{
		return family_;
	}

	int type()
	{
		return SOCK_STREAM;
	}

	int protocol() const
	{
		return IPPROTO_TCP;
	}

private:
	int family_;
};

}	// namespace ip
}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_IP_TCP_HPP__
