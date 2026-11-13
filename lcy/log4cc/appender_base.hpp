#ifndef __LCY_LOG4CC_APPENDER_BASE_HPP__
#define __LCY_LOG4CC_APPENDER_BASE_HPP__

#include <memory>

#include "lcy/noncopyable.hpp"
#include "lcy/log4cc/record.hpp"
#include "lcy/log4cc/formatter.hpp"

namespace lcy
{
namespace log4cc
{

class appender_base :
    private noncopyable
{
public:
    typedef std::shared_ptr<appender_base> sptr_type;

public:
    formatter::sptr_type get_formatter()
    {
        return formatter_;
    }

    void set_formatter(formatter::sptr_type f)
    {
        formatter_ = f;
    }

    virtual void reload() = 0;
    virtual void log(record::sptr_type rsptr) = 0;

protected:
    formatter::sptr_type formatter_;
};

}   // namespace log4cc
}   // namespace lcy

#endif  // __LCY_LOG4CC_APPENDER_BASE_HPP__