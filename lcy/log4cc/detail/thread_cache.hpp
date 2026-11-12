#ifndef __LCY_LOG4CC_DETAIL_THREAD_CACHE_HPP__
#define __LCY_LOG4CC_DETAIL_THREAD_CACHE_HPP__

namespace lcy
{
namespace log4cc
{
namespace detail
{

namespace thread_cache
{

inline pid_t pid()
{
    thread_local pid_t value = ::getpid();
    return value;
}

inline long tid()
{
    thread_local long value = ::syscall(SYS_gettid);
    return value;
}

}   // namespace thread_cache

}   // namespace detail
}   // namespace log4cc
}   // namespace lcy

#endif  // __LCY_LOG4CC_DETAIL_THREAD_CACHE_HPP__