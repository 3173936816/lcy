#ifndef __LCY_LOG4CC_DETAIL_BASIC_CONSOLE_SINK_HPP__
#define __LCY_LOG4CC_DETAIL_BASIC_CONSOLE_SINK_HPP__

#include <mutex>
#include <memory>
#include <iostream>

namespace lcy
{
namespace log4cc
{
namespace detail
{

template <
    typename Mutex,
    typename SinkBase
>
class basic_console_sink :
    public SinkBase
{
public:
    typedef Mutex mutex_type;
    typedef SinkBase base_type;
    typedef std::shared_ptr<basic_console_sink> sptr_type;

public:
    void reload() override
    {
        // nothing to do
    }

    void flush() override
    {
        std::lock_guard<mutex_type> locker(mutex_);
        std::cout << std::flush;
    }

    void log(record::sptr_type rsptr) override
    {
        if ( base_type::formatter_ ) {
            std::string message = base_type::formatter_->format(rsptr);

            std::lock_guard<mutex_type> locker(mutex_);
            std::cout << message;
        }
    }

private:
    mutex_type mutex_;
};

}   // namespace detail
}   // namespace log4cc
}   // namespace lcy

#endif  //  __LCY_LOG4CC_DETAIL_BASIC_CONSOLE_SINK_HPP__