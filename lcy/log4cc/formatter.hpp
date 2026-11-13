#ifndef __LCY_LOG4CC_FORMATTER_HPP__
#define __LCY_LOG4CC_FORMATTER_HPP__

#include <list>
#include <memory>
#include <sstream>
#include <string>

#include "lcy/noncopyable.hpp"
#include "lcy/log4cc/record.hpp"
#include "lcy/log4cc/detail/items.hpp"

namespace lcy
{
namespace log4cc
{

class formatter :
    private noncopyable
{
public:
    typedef std::shared_ptr<formatter> sptr_type;

public:
    formatter() :
        pattern_("[%d{%Y-%m-%d %H:%M:%S}] [%l] [pid:%p] [tid:%t] [%f:%n] [%c] %m%E")
    {
        detail::parse_pattern<record::sptr_type>(pattern_, items_);
    }

    explicit formatter(std::string pattern) :
        pattern_(std::move(pattern))
    {
        detail::parse_pattern<record::sptr_type>(pattern_, items_);
    }

    const std::string& get_pattern() const
    {
        return pattern_;
    }

    std::string format(record::sptr_type rsptr) const
    {
        std::ostringstream oss;

        for ( const auto& item : items_ ) {
            item->format(rsptr, oss);
        }

        return oss.str();
    }

private:
    typedef typename detail::item_base<
        record::sptr_type
    >::sptr_type item_sptr_type;

    std::string pattern_;
    std::list<item_sptr_type> items_;
};

}   // namespace log4cc
}   // namespace lcy

#endif  // __LCY_LOG4CC_FORMATTER_HPP__