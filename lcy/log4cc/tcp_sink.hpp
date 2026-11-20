#ifndef __LCY_LOG4CC_TCP_SINK_HPP__
#define __LCY_LOG4CC_TCP_SINK_HPP__

#include "lcy/asio/ip/tcp.hpp"
#include "lcy/log4cc/sink_base.hpp"
#include "lcy/log4cc/detail/basic_tcp_sink.hpp"

namespace lcy
{
namespace log4cc
{

typedef detail::basic_tcp_sink<
    lcy::asio::ip::tcp::socket,
    sink_base
> tcp_sink_mt;

typedef detail::basic_tcp_sink<
    lcy::asio::ip::tcp::socket,
    sink_base
> tcp_sink_st;

}   // namespace log4cc
}   // namespace lcy

#endif  // __LCY_LOG4CC_TCP_SINK_HPP__