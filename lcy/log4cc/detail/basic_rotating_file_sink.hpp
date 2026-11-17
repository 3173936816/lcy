#ifndef __LCY_LOG4CC_DETAIL_BASIC_ROTATING_FILE_SINK_HPP__
#define __LCY_LOG4CC_DETAIL_BASIC_ROTATING_FILE_SINK_HPP__

#include <mutex>
#include <memory>
#include <string>
#include <fstream>

#include "lcy/log4cc/detail/file_ops.hpp"
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
class basic_rotating_file_sink :
    public SinkBase
{
public:
    typedef Mutex mutex_type;
    typedef SinkBase base_type;

    typedef std::shared_ptr<basic_rotating_file_sink> sptr_type;
    typedef typename base_type::record_sptr_type record_sptr_type;

public:
    basic_rotating_file_sink(std::string full_path, std::size_t size_limit) :
        size_limit_(size_limit), written_(0), full_path_(std::move(full_path))
    {
        open_file();
        written_ = file_ops::file_size(full_path_);
    }

    void reload() override
    {
        std::lock_guard<mutex_type> locker(mutex_);

        if ( ofs_.is_open() ) {
            ofs_.close();
        }

        open_file();
        written_ = file_ops::file_size(full_path_);
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

            if ( written_ + message.size() > size_limit_ ) {
                rotate();
            }

            ofs_ << message;
            written_ += message.size();
        }
    }

private:
    void open_file()
    {
        ofs_.open(full_path_, std::ios_base::app);

        if ( !ofs_.is_open() ) {
            lcy::system::error_code ec(errno);
            throw_log4cc_exception(ec, "basic_rotating_file_sink::open_file");
        }
    }

    void rotate()
    {
        if ( ofs_.is_open() ) {
            ofs_.close();
        }

        std::size_t file_index = file_ops::scan_file_index(full_path_) + 1;

        std::string archived = archived_name(file_index);
        file_ops::renamefile(full_path_, archived);

        open_file();
        written_ = 0;
    }

    std::string archived_name(std::size_t index) const
    {
        std::string base = file_ops::filename(full_path_);

        std::size_t slash = full_path_.find_last_of('/');
        std::string dir;

        if ( slash != std::string::npos ) {
            dir = full_path_.substr(0, slash + 1);
        }

        return dir + base + "." + std::to_string(index);
    }

private:
    mutex_type mutex_;
    std::size_t size_limit_;
    std::size_t written_;
    std::string full_path_;
    std::ofstream ofs_;
};

}   // namespace detail
}   // namespace log4cc
}   // namespace lcy

#endif  // __LCY_LOG4CC_DETAIL_BASIC_ROTATING_FILE_SINK_HPP__