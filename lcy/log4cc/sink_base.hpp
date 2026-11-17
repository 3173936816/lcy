#ifndef __LCY_LOG4CC_SINK_BASE_HPP__
#define __LCY_LOG4CC_SINK_BASE_HPP__

#include <memory>

#include "lcy/noncopyable.hpp"
#include "lcy/log4cc/record.hpp"
#include "lcy/log4cc/formatter.hpp"

namespace lcy
{
namespace log4cc
{

class sink_base :
    private noncopyable
{
public:
    typedef record::sptr_type record_sptr_type;
    typedef std::shared_ptr<sink_base> sptr_type;

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
    virtual void flush() = 0;
    virtual void log(record_sptr_type rsptr) = 0;

protected:
    formatter::sptr_type formatter_;
};

}   // namespace log4cc
}   // namespace lcy

#endif  // __LCY_LOG4CC_SINK_BASE_HPP__