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

void parse_pattern(const std::string& pattern, std::list<item_base::sptr_type>& items);

}   // namespace detail
}   // namespace log4cc
}   // namespace lcy

#include "lcy/log4cc/detail/impl/items.ipp"

#endif  // __LCY_LOG4CC_DETAIL_ITEMS_HPP__