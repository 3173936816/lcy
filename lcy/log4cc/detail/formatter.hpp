#ifndef __LCY_LOG4CC_DETAIL_FORMATTER_HPP__
#define __LCY_LOG4CC_DETAIL_FORMATTER_HPP__

#include <list>
#include <string>
#include <sstream>

#include "lcy/noncopyable.hpp"
#include "lcy/log4cc/level.hpp"
#include "lcy/log4cc/detail/event.hpp"

namespace lcy
{
namespace log4cc
{
namespace detail
{

class formatter :
    private noncopyable
{
public:
    class item
    {
    public:
        virtual ~item() {}
        virtual void get_item(event& e, std::ostringstream& oss) const = 0;
    };

public:
    inline formatter();
    inline formatter(std::string pattern);
    inline ~formatter();

    inline const std::string& get_pattern() const;
    inline void set_pattern(std::string pattern);

    inline std::string format(event& e) const;

private:
    inline void parse_pattern();
    inline void clear_items();

private:
    std::string pattern_;
    std::list<item*> items_;
};

}   // namespace detail
}   // namespace log4cc
}   // namespace lcy

#include "lcy/log4cc/detail/impl/formatter.ipp"

#endif  // __LCY_LOG4CC_DETAIL_FORMATTER_HPP__