#ifndef __LCY_LOG4CC_ROTATING_FILE_APPENDER_MT_HPP__
#define __LCY_LOG4CC_ROTATING_FILE_APPENDER_MT_HPP__

#include <mutex>

#include "lcy/log4cc/appender_base.hpp"
#include "lcy/log4cc/detail/rotating_file_appender.hpp"

namespace lcy
{
namespace log4cc
{

typedef detail::rotating_file_appender<
	std::mutex,
	appender_base
> rotating_file_appender_mt;

}	// namespace log4cc
}	// namespace lcy

#endif	// __LCY_LOG4CC_ROTATING_FILE_APPENDER_MT_HPP__
