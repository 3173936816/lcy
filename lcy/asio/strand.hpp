#ifndef __LCY_ASIO_STRAND_HPP__
#define __LCY_ASIO_STRAND_HPP__

#include <functional>

#include "lcy/noncopyable.hpp"
#include "lcy/asio/io_context.hpp"

namespace lcy
{
namespace asio
{

class strand :
    private noncopyable
{
public:
    typedef std::function<void()> func_type;

public:
    strand(io_context& ioc) : ioc_(ioc) {}

    void post(func_type func)
    {
        std::lock_guard<std::mutex> locker(mutex_);

        bool is_empty = serial_queue_.empty();
        serial_queue_.push_back(std::move(std::move(func)));

        if ( is_empty ) {
            serial_execute();
        }
    }

private:
    void serial_execute()
    {
        lcy::asio::post(ioc_, [this](){
            func_type func;

            {
                std::lock_guard<std::mutex> locker(mutex_);
                if ( !serial_queue_.empty() ) {
                    func = std::move(serial_queue_.front());
                    serial_queue_.pop_front();
                }
            }

            if ( func ) {
                func();
            }

            {
                std::lock_guard<std::mutex> locker(mutex_);
                if ( !serial_queue_.empty() ) {
                    serial_execute();
                }
            }
        });
    }

private:
    io_context& ioc_;
    std::mutex mutex_;
    std::list<func_type> serial_queue_;
};

template <typename... Args>
std::function<void(Args...)> strand_wrapper(strand& s,
    std::function<void(Args...)> func)
{
    return [&s, func](Args... args) {
        auto task = [func, args...]() {
            func(args...);
        };
        s.post(std::move(task));
    };
}

}   // namespace asio
}   // namespace lcy

#endif  // __LCY_ASIO_STRAND_HPP__