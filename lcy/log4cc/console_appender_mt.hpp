#ifndef __LCY_LOG4CC_CONSOLE_APPENDER_MT_HPP__
#define __LCY_LOG4CC_CONSOLE_APPENDER_MT_HPP__

#include <mutex>

#include "lcy/log4cc/appender_base.hpp"
#include "lcy/log4cc/detail/console_appender.hpp"

namespace lcy
{
namespace lo4cc
{

typedef detail::console_appender<
	std::mutex,
	appender_base
> console_appender_mt;

}	// namespace log4cc
}	// namespace lcy

#endif	// __LCY_LOG4CC_CONSOLE_APPENDER_MT_HPP__
