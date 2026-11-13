#ifndef __LCY_LOG4CC_DETAIL_FILE_APPENDER_HPP__
#define __LCY_LOG4CC_DETAIL_FILE_APPENDER_HPP__

#include <mutex>
#include <memory>
#include <fstream>

#include "lcy/log4cc/detail/throw_log4cc_exception.hpp"

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
class file_appender :
    public AppenderBase
{
public:
    typedef Mutex mutex_type;
    typedef AppenderBase base_type;
    typedef std::shared_ptr<file_appender> sptr_type;

public:
    file_appender(std::string file_name) :
        file_name_(std::move(file_name))
    {
        open_file();
    }

    void reload() override 
    {
        std::lock_guard<mutex_type> locker(mutex_);

        if ( ofs_.is_open() ) {
            ofs_.close();
        }

        open_file();
    }

    void log(record::sptr_type rsptr) override
    {
        if ( base_type::formatter_ ) {
            std::string message = base_type::formatter_->format(rsptr);

            std::lock_guard<Mutex> locker(mutex_);
            ofs_ << message;
        }
    }

private:
    void open_file()
    {
        ofs_.open(file_name_, std::io_base::app);
        if ( !ofs_.is_open() ) {
            lcy::system::error_code ec(errno);
            throw_log4cc_exception(ec, "file_appender::open_file");
        }
    }

private:
    mutex_type mutex_;
    std::string file_name_;
    std::ofstream ofs_;
};

}   // namespace detail
}   // namespace log4cc
}   // namespace lcy

#endif  // __LCY_LOG4CC_DETAIL_FILE_APPENDER_HPP__