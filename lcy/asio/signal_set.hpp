#ifndef __LCY_ASIO_SIGNAL_SET_HPP__
#define __LCY_ASIO_SIGNAL_SET_HPP__

#include "lcy/asio/io_context.hpp"
#include "lcy/asio/detail/basic_signal_set.hpp"

namespace lcy
{
namespace asio
{

typedef detail::basic_signal_set<
    io_context
> signal_set;

}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_SIGNAL_SET_HPP__
