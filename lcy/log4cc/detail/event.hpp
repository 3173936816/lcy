#ifndef __LCY_LOG4CC_DETAIL_EVENT_HPP__
#define __LCY_LOG4CC_DETAIL_EVENT_HPP__

#include <string>
#include <sstream>
#include <cstddef>
#include <ctime>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>

#include "lcy/log4cc/level.hpp"

namespace lcy
{
namespace log4cc
{
namespace detail
{

inline pid_t current_pid() {
    thread_local pid_t pid = ::getpid();
    return pid;
}

inline pthread_t current_thread_id() {
    thread_local pthread_t tid = ::pthread_self();
    return tid;
}

struct event
{
    level level_;
    pid_t pid_;
    pthread_t thread_id_;
    std::time_t timestamp_;
    std::string file_name_;
    std::size_t file_line_;
    std::string logger_name_;
    std::ostringstream oss_;

    event(level l, std::string file_name, std::size_t file_line, std::string logger_name) :
        level_(l),
        pid_(current_pid()),
        thread_id_(current_thread_id()),
        timestamp_(std::time(0)),
        file_name_(std::move(file_name)),
        file_line_(file_line),
        logger_name_(std::move(logger_name)) {}
};

} // namespace detail
} // namespace log4cc
} // namespace lcy

#endif