#ifndef __LCY_LOG4CC_DETAIL_ROTATING_FILE_APPENDER_HPP__
#define __LCY_LOG4CC_DETAIL_ROTATING_FILE_APPENDER_HPP__

#include <mutex>
#include <memory>
#include <string>
#include <fstream>
#include <functional>

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
    typename AppenderBase
>
class rotating_file_appender :
    public AppenderBase
{
public:
    typedef std::function<
        void (
            const std::string& archived_file_name
        )
    > rotate_func_type;

    typedef Mutex mutex_type;
    typedef AppenderBase base_type;
    typedef std::shared_ptr<rotating_file_appender> sptr_type;

public:
    rotating_file_appender(std::string full_path, std::size_t size_limit) :
        size_limit_(size_limit), written_(0), full_path_(std::move(full_path))
    {
        open_file();
        written_ = file_ops::file_size(full_path_);
    }

    void set_rotate_callback(rotate_func_type func)
    {
        func_ = std::move(func);
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

    void log(record::sptr_type rsptr) override
    {
        std::string archived_file_name;

        if ( base_type::formatter_ ) {
            std::string message = base_type::formatter_->format(rsptr);

            std::lock_guard<mutex_type> locker(mutex_);

            if ( written_ + message.size() > size_limit_ ) {
                archived_file_name = rotate();
            }

            ofs_ << message;
            written_ += message.size();
        }

        if ( !archived_file_name.empty() && func_ ) {
            func_(archived_file_name);
        }
    }

private:
    void open_file()
    {
        ofs_.open(full_path_, std::ios_base::app);

        if ( !ofs_.is_open() ) {
            lcy::system::error_code ec(errno);
            throw_log4cc_exception(ec, "rotating_file_appender::open_file");
        }
    }

    std::string rotate()
    {
        if ( ofs_.is_open() ) {
            ofs_.close();
        }

        std::size_t file_index = file_ops::scan_file_index(full_path_) + 1;

        std::string archived = archived_name(file_index);
        file_ops::renamefile(full_path_, archived);

        open_file();
        written_ = 0;

        return archived;
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
    rotate_func_type func_;
};

}   // namespace detail
}   // namespace log4cc
}   // namespace lcy

#endif  // __LCY_LOG4CC_DETAIL_ROTATING_FILE_APPENDER_HPP__