#ifndef __LCY_ASIO_STEADY_TIMER_HPP__
#define __LCY_ASIO_STEADY_TIMER_HPP__

#include "lcy/asio/io_context.hpp"
#include "lcy/asio/detail/steady_timer_service.hpp"
#include "lcy/asio/detail/basic_waitable_timer.hpp"

namespace lcy
{
namespace asio
{

typedef detail::basic_waitable_timer<
	io_context,
	detail::steady_timer_service
> steady_timer; 

}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_STEADY_TIMER_HPP__
