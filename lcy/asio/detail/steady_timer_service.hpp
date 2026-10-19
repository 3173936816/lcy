#ifndef __LCY_ASIO_DETAIL_STEADY_TIMER_SERVICE_HPP__
#define __LCY_ASIO_DETAIL_STEADY_TIMER_SERVICE_HPP__

#include "lcy/asio/detail/basic_timer_service.hpp"

namespace lcy
{
namespace asio
{
namespace detail
{

struct steady_clock
{
    static long long now()
    {
        timespec ts;
        ::clock_gettime(CLOCK_MONOTONIC, &ts);

        return ts.tv_sec * 1000
            + ts.tv_nsec / 1000000;
    }
};

typedef basic_timer_service<steady_clock> steady_timer_service;

}	// namespace detail
}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_DETAIL_STEADY_TIMER_SERVICE_HPP__
