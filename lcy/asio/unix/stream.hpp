#ifndef __LCY_ASIO_UNIX_STREAM_SOCKET_HPP__
#define __LCY_ASIO_UNIX_STREAM_SOCKET_HPP__

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include "lcy/asio/io_context.hpp"
#include "lcy/asio/detail/basic_unix_endpoint.hpp"
#include "lcy/asio/detail/basic_stream_socket.hpp"
#include "lcy/asio/detail/basic_acceptor.hpp"

namespace lcy
{
namespace asio
{
namespace unix
{

class stream
{
public:
	typedef detail::basic_unix_endpoint<
		stream> endpoint;

	typedef detail::basic_stream_socket<
		io_context, stream, endpoint> socket;

	typedef detail::basic_acceptor<
		io_context, stream, endpoint> acceptor;

	int family() const
	{
		return AF_UNIX;
	}

	int type() const
	{
		return SOCK_STREAM;
	}

	int protocol() const
	{
		return 0;
	}
};

} 	// namespace unix
}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_UNIX_STREAM_SOCKET_HPP__
