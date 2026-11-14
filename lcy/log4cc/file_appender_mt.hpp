#ifndef __LCY_LOG4CC_FILE_APPENDER_MT_HPP__
#define __LCY_LOG4CC_FILE_APPENDER_MT_HPP__

#include <mutex>

#include "lcy/log4cc/appender_base.hpp"
#include "lcy/log4cc/detail/file_appender.hpp"

namespace lcy
{
namespace lo4cc
{

typedef detail::file_appender<
	std::mutex,
	appender_base
> file_appender_mt;

}	// namespace log4cc
}	// namespace lcy

#endif	// __LCY_LOG4CC_FILE_APPENDER_MT_HPP__
