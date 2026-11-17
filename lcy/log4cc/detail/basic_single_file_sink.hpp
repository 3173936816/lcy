#ifndef __LCY_LOG4CC_DETAIL_BASIC_SINGLE_FILE_SINK_HPP__
#define __LCY_LOG4CC_DETAIL_BASIC_SINGLE_FILE_SINK_HPP__

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
    typename SinkBase
>
class basic_single_file_sink :
    public SinkBase
{
public:
    typedef Mutex mutex_type;
    typedef SinkBase base_type;

    typedef std::shared_ptr<basic_single_file_sink> sptr_type;
    typedef typename base_type::record_sptr_type record_sptr_type;

public:
    basic_single_file_sink(std::string file_name) :
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

    void flush() override
    {
        std::lock_guard<mutex_type> locker(mutex_);

        if ( ofs_.is_open() ) {
            ofs_ << std::flush;
        }
    }

    void log(record_sptr_type rsptr) override
    {
        if ( base_type::formatter_ ) {
            std::string message = base_type::formatter_->format(rsptr);

            std::lock_guard<mutex_type> locker(mutex_);
            ofs_ << message;
        }
    }

private:
    void open_file()
    {
        ofs_.open(file_name_, std::ios_base::app);
        if ( !ofs_.is_open() ) {
            lcy::system::error_code ec(errno);
            throw_log4cc_exception(ec, "basic_single_file_sink::open_file");
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

#endif  // __LCY_LOG4CC_DETAIL_BASIC_SINGLE_FILE_SINK_HPP__