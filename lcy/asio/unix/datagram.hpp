#ifndef __LCY_ASIO_UNIX_DATAGRAM_SOCKET_HPP__
#define __LCY_ASIO_UNIX_DATAGRAM_SOCKET_HPP__

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include "lcy/asio/io_context.hpp"
#include "lcy/asio/detail/unix_endpoint.hpp"
#include "lcy/asio/detail/basic_datagram_socket.hpp"

namespace lcy
{
namespace asio
{
namespace unix
{

class datagram
{
public:
	typedef detail::unix_endpoint endpoint;

	typedef detail::basic_datagram_socket<
		io_context, datagram, endpoint> socket;

	int family() const
	{
		return AF_UNIX;
	}

	int type()
	{
		return SOCK_DGRAM;
	}

	int protocol() const
	{
		return 0;
	}
};

} 	// namespace unix
}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_UNIX_DATAGRAM_SOCKET_HPP__
