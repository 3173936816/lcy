#ifndef __LCY_ASIO_SYSTEM_TIMER_HPP__
#define __LCY_ASIO_SYSTEM_TIMER_HPP__

#include "lcy/asio/io_context.hpp"
#include "lcy/asio/detail/system_timer_service.hpp"
#include "lcy/asio/detail/basic_waitable_timer.hpp"

namespace lcy
{
namespace asio
{

typedef detail::basic_waitable_timer<
	io_context,
	detail::system_timer_service
> system_timer; 

}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_SYSTEM_TIMER_HPP__
