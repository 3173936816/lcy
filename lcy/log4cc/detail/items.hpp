#ifndef __LCY_LOG4CC_DETAIL_ITEMS_HPP__
#define __LCY_LOG4CC_DETAIL_ITEMS_HPP__

#include <list>
#include <memory>
#include <sstream>

#include "lcy/noncopyable.hpp"

namespace lcy
{
namespace log4cc
{
namespace detail
{

template <typename RecordSptr>
class item_base :
    private noncopyable
{
public:
    typedef std::shared_ptr<item_base> sptr_type;

    virtual ~item_base() {}
    virtual void format(RecordSptr rsptr, std::ostringstream& oss) = 0;
};

/*
 *    %l	level
 *    %p	pid
 *    %t	thread_id
 *    %d	timestamp
 *    %f	file_name
 *    %n	file_line
 *    %c	logger_name
 *    %m	message
 *    %T	tab
 *    %E	enter
 *    %%	literal
 * 
 * default: "[%d{%Y-%m-%d %H:%M:%S}] [%l] [pid:%p] [tid:%t] [%f:%n] [%c] %m%E"
 */

template <typename RecordSptr>
void parse_pattern(const std::string& pattern,
    std::list<typename item_base<RecordSptr>::sptr_type>& items);

}   // namespace detail
}   // namespace log4cc
}   // namespace lcy

#include "lcy/log4cc/detail/impl/items.ipp"

#endif  // __LCY_LOG4CC_DETAIL_ITEMS_HPP__