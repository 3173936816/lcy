#ifndef __LCY_ASIO_WORK_GUARD_HPP__
#define __LCY_ASIO_WORK_GUARD_HPP__

#include "lcy/noncopyable.hpp"
#include "lcy/asio/io_context.hpp"

namespace lcy
{
namespace asio
{

class work_guard :
    private noncopyable
{
public:
    work_guard(io_context& ioc) :
        ioc_(ioc), is_owner_(true)
    {
        ioc_.sch_.operation_increase();
    }

    ~work_guard()
    {
        reset();
    }

    void reset()
    {
        if ( is_owner_ ) {
            ioc_.sch_.operation_decrease();
            is_owner_ = false;
        }
    }

    bool is_owner() const
    {
        return is_owner_;
    }

private:
    io_context& ioc_;
    bool is_owner_;
};

}   // namespace asio
}   // namespace lcy

#endif  // __LCY_ASIO_WORK_GUARD_HPP__
