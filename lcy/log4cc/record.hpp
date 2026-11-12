#ifndef __LCY_LOG4CC_RECORD_HPP__
#define __LCY_LOG4CC_RECORD_HPP__

#include <cstddef>
#include <ctime>
#include <memory>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "lcy/noncopyable.hpp"
#include "lcy/log4cc/level.hpp"
#include "lcy/log4cc/detail/thread_cache.hpp"

namespace lcy
{
namespace log4cc
{

class record :
    private noncopyable
{
public:
    typedef std::shared_ptr<record> sptr_type;

public:
    record(level l,
           std::string file_name,
           std::size_t file_line,
           std::string logger_name) :
        level_(l),
        pid_(detail::thread_cache::pid()),
        thread_id_(detail::thread_cache::tid()),
        timestamp_(std::time(0)),
        file_name_(std::move(file_name)),
        file_line_(file_line),
        logger_name_(std::move(logger_name)) {}

    level get_level() const
    {
        return level_;
    }

    pid_t get_pid() const
    {
        return pid_;
    }

    long get_thread_id() const
    {
        return thread_id_;
    }

    std::time_t get_timestamp() const
    {
        return timestamp_;
    }

    const std::string& get_file_name() const
    {
        return file_name_;
    }

    std::size_t get_file_line() const
    {
        return file_line_;
    }

    const std::string& get_logger_name() const
    {
        return logger_name_;
    }

    std::string get_message() const
    {
        return oss_.str();
    }

    const std::ostringstream& get_oss() const
    {
        return oss_;
    }

private:
    level level_;
    pid_t pid_;
    long thread_id_;
    std::time_t timestamp_;
    std::string file_name_;
    std::size_t file_line_;
    std::string logger_name_;
    std::ostringstream oss_;
};

}   // namespace log4cc
}   // namespace lcy

#endif  // __LCY_LOG4CC_RECORD_HPP__