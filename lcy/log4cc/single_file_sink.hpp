#ifndef __LCY_LOG4CC_SINGLE_FILE_SINK_HPP__
#define __LCY_LOG4CC_SINGLE_FILE_SINK_HPP__

#include <mutex>

#include "lcy/log4cc/sink_base.hpp"
#include "lcy/log4cc/detail/null_mutex.hpp"
#include "lcy/log4cc/detail/basic_single_file_sink.hpp"

namespace lcy
{
namespace log4cc
{

typedef detail::basic_single_file_sink<
	std::mutex,
	sink_base
> single_sink_mt;

typedef detail::basic_single_file_sink<
	detail::null_mutex,
	sink_base
> single_sink_st;

}	// namespace log4cc
}	// namespace lcy

#endif	// __LCY_LOG4CC_SINGLE_FILE_SINK_HPP__
