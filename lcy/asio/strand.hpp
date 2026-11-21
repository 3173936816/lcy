#ifndef __LCY_ASIO_STRAND_HPP__
#define __LCY_ASIO_STRAND_HPP__

#include <thread>
#include <functional>

#include "lcy/asio/io_context.hpp"
#include "lcy/asio/detail/basic_strand.hpp"

namespace lcy
{
namespace asio
{

typedef detail::basic_strand<
    io_context
> strand;

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
