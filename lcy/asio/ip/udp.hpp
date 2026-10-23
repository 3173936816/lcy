#ifndef __LCY_ASIO_IP_UDP_HPP__
#define __LCY_ASIO_IP_UDP_HPP__

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include "lcy/asio/io_context.hpp"
#include "lcy/asio/detail/basic_endpoint.hpp"
#include "lcy/asio/detail/basic_datagram_socket.hpp"
#include "lcy/asio/detail/basic_resolver.hpp"

namespace lcy
{
namespace asio
{
namespace ip
{

class udp
{
private:
	udp(int family) : family_(family) {}

public:
	typedef detail::basic_endpoint<udp> endpoint;

	typedef detail::basic_datagram_socket<
		io_context, udp, endpoint> socket;

	typedef detail::basic_resolver<udp, endpoint> resolver;

	static udp v4()
	{
		return udp(AF_INET);
	}

	static udp v6()
	{
		return udp(AF_INET6);
	}

	int family() const
	{
		return family_;
	}

	int type() const
	{
		return SOCK_DGRAM;
	}

	int protocol() const
	{
		return IPPROTO_UDP;
	}

private:
	int family_;
};

}	// namespace ip
}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_IP_UDP_HPP__
