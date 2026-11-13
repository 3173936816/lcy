#ifndef __LCY_LOG4CC_DETAIL_CONSOLE_APPENDER_HPP__
#define __LCY_LOG4CC_DETAIL_CONSOLE_APPENDER_HPP__

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
    typename AppenderBase
>
class console_appender :
    public AppenderBase
{
public:
    typedef Mutex mutex_type;
    typedef AppenderBase base_type;
    typedef std::shared_ptr<console_appender> sptr_type;

public:
    void reload() override
    {
        // nothing to do
    }

    void log(record::sptr_type rsptr) override
    {
        if ( base_type::formatter_ ) {
            std::string message = base_type::formatter_->format(rsptr);

            std::lock_guard<mutex_type> locker(mutex_);
            std::cout << message;
        }
    }

public:
    mutex_type mutex_;
};

}   // namespace detail
}   // namespace log4cc
}   // namespace lcy

#endif  //  __LCY_LOG4CC_DETAIL_CONSOLE_APPENDER_HPP__