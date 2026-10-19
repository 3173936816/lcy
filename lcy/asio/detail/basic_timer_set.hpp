#ifndef __LCY_ASIO_DETAIL_BASIC_TIMER_SET_HPP__
#define __LCY_ASIO_DETAIL_BASIC_TIMER_SET_HPP__

#include "lcy/asio/detail/timer_set_base.hpp"

namespace lcy
{
namespace asio
{
namespace detail
{

template <typename Clock>
class basic_timer_set :
	public timer_set_base
{
public:
	long long get_wait_ms() override
	{
		if ( timer_set_.empty() ) {
			return -1;
		}

		internal_timer* earliest_timer = *timer_set_.begin();
		long long now_time = Clock::now();

		if ( now_time >= earliest_timer->expiry_ ) {
			return 0;
		}
	
		return earliest_timer->expiry_ - now_time;
	}

	void get_ready_internal_timer_ops(std::list<operation>& op_list) override
	{
		internal_timer tag;
		tag.expiry_ = Clock::now();

		auto beg_iter = timer_set_.begin();
		auto end_iter = timer_set_.upper_bound(&tag);
		for ( auto iter = beg_iter; iter != end_iter; ++iter ) {
			internal_timer* expired_timer = *iter;
			
			timer_set_index_.erase(expired_timer);
			op_list.push_back(std::move(expired_timer->timer_op_));
		}
		timer_set_.erase(beg_iter, end_iter);
	}
};

}	// namespace detail
}	// anmespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_DETAIL_BASIC_TIMER_SET_HPP__
