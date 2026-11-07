#ifndef __LCY_ASIO_SSL_STREAM_SOCKET_HPP__
#define __LCY_ASIO_SSL_STREAM_SOCKET_HPP__

#include "lcy/asio/ssl/context.hpp"
#include "lcy/asio/ssl/detail/basic_stream_socket.hpp"

namespace lcy
{
namespace asio
{
namespace ssl
{

typedef detail::basic_stream_socket<
    context
> stream_socket;

}   // namespace ssl
}   // namespace asio
}   // namespace lcy

#endif  // __LCY_ASIO_SSL_STREAM_SOCKET_HPP__