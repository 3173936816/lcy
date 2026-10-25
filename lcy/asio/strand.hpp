#ifndef __LCY_ASIO_STRAND_HPP__
#define __LCY_ASIO_STRAND_HPP__

#include <thread>
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
    strand(io_context& ioc) :
        ioc_(ioc), executing_(false) {}

    void post(func_type func)
    {
        bool need_execute = false;
        {
            std::lock_guard<std::mutex> locker(mutex_);
            serial_queue_.push_back(std::move(func));
            if ( !executing_ ) {
                executing_ = true;
                need_execute = true;
            }
        }
        if ( need_execute ) {
            serial_execute();
        }
    }

private:
    void serial_execute()
    {
        lcy::asio::post(ioc_, [this]() {
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

            bool has_more = false;
            {
                std::lock_guard<std::mutex> locker(mutex_);
                if ( !serial_queue_.empty() ) {
                    has_more = true;
                } else {
                    executing_ = false;
                }
            }

            if ( has_more ) {
                serial_execute();
            }
        });
    }

private:
    io_context& ioc_;
    std::mutex mutex_;
    bool executing_;
    std::list<func_type> serial_queue_;
};

namespace detail
{

template <typename Function>
class strand_wrapper_impl
{
public:
    strand_wrapper_impl(strand& s, Function func) :
        strand_(&s), func_(std::move(func)) {}

    template <typename... Args>
    void operator()(Args&&... args) const
    {
        Function func = func_; 
        auto task = [func, args...]() {
            func(args...);
        };
        strand_->post(std::move(task));
    }

private:
    strand* strand_;
    Function func_;
};

}   // namespace detail

template <typename Function>
detail::strand_wrapper_impl<Function> strand_wrapper(strand& s, Function func)
{
    return detail::strand_wrapper_impl<Function>(s, std::move(func));
}

}   // namespace asio
}   // namespace lcy

#endif  // __LCY_ASIO_STRAND_HPP__
