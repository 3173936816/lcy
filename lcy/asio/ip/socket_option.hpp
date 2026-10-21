#ifndef __LCY_ASIO_IP_SOCKET_OPTION_HPP__
#define __LCY_ASIO_IP_SOCKET_OPTION_HPP__

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "lcy/asio/detail/basic_socket_option.hpp"

namespace lcy
{
namespace asio
{
namespace ip
{

namespace socket_option
{

// reuse
typedef detail::basic_socket_option<SOL_SOCKET, SO_REUSEADDR, int> reuse_address;
typedef detail::basic_socket_option<SOL_SOCKET, SO_REUSEPORT, int> reuse_port;

// keepalive and broadcast
typedef detail::basic_socket_option<SOL_SOCKET, SO_KEEPALIVE, int> keep_alive;
typedef detail::basic_socket_option<SOL_SOCKET, SO_BROADCAST, int> broadcast;

// buffer
typedef detail::basic_socket_option<SOL_SOCKET, SO_RCVBUF, int> receive_buffer_size;
typedef detail::basic_socket_option<SOL_SOCKET, SO_SNDBUF, int> send_buffer_size;

// error
typedef detail::basic_socket_option<SOL_SOCKET, SO_ERROR, int> error;

// tcp
typedef detail::basic_socket_option<IPPROTO_TCP, TCP_NODELAY, int> tcp_nodelay;

}	// namespace socket_option

}	// namespace ip
}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_IP_SOCKET_OPTION_HPP__
