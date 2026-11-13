#ifndef __LCY_LOG4CC_DETAIL_NULL_MUTEX_HPP__
#define __LCY_LOG4CC_DETAIL_NULL_MUTEX_HPP__

#include "lcy/noncopyable.hpp"

namespace lcy
{
namespace log4cc
{
namespace detail
{

class null_mutex :
    public noncopyable
{
public:
    void lock() {}
    void unlock() {}
};


}   // namespace detail
}   // namespace log4cc
}   // namespace lcy

#endif  // __LCY_LOG4CC_DETAIL_NULL_MUTEX_HPP__