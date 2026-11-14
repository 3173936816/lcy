#ifndef __LCY_LOG4CC_ROTATING_FILE_APPENDER_ST_HPP__
#define __LCY_LOG4CC_ROTATING_FILE_APPENDER_ST_HPP__

#include "lcy/log4cc/appender_base.hpp"
#include "lcy/log4cc/detail/null_mutex.hpp"
#include "lcy/log4cc/detail/rotating_file_appender.hpp"

namespace lcy
{
namespace log4cc
{

typedef detail::rotating_file_appender<
	detail::null_mutex,
	appender_base
> rotating_file_appender_st;

}	// namespace log4cc
}	// namespace lcy

#endif	// __LCY_LOG4CC_ROTATING_FILE_APPENDER_ST_HPP__
