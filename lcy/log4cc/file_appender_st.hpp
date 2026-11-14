#ifndef __LCY_LOG4CC_FILE_APPENDER_ST_HPP__
#define __LCY_LOG4CC_FILE_APPENDER_ST_HPP__

#include "lcy/log4cc/appender_base.hpp"
#include "lcy/log4cc/detail/null_mutex.hpp"
#include "lcy/log4cc/detail/file_appender.hpp"

namespace lcy
{
namespace lo4cc
{

typedef detail::file_appender<
	detail::null_mutex,
	appender_base
> file_appender_st;

}	// namespace log4cc
}	// namespace lcy

#endif	// __LCY_LOG4CC_FILE_APPENDER_ST_HPP__
