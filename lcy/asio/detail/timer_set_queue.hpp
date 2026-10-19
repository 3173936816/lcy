#ifndef __LCY_ASIO_DETAIL_TIMER_SET_QUEUE_HPP__
#define __LCY_ASIO_DETAIL_TIMER_SET_QUEUE_HPP__

#include <list>

#include "lcy/noncopyable.hpp"
#include "lcy/object_pool.hpp"
#include "lcy/asio/detail/timer_set_base.hpp"

namespace lcy
{
namespace asio
{
namespace detail
{

class timer_set_queue :
	private noncopyable
{
public:
	typedef timer_set_base::internal_timer internal_timer;

	internal_timer* alloc()
	{
		return timer_datas_.alloc();
	}

	void free(internal_timer* t)
	{
		timer_datas_.free(t);
	}

	void add_timer_set(timer_set_base* ts)
	{
		timer_sets_.push_back(ts);
	}

	void remove_timer_set(timer_set_base* ts)
	{
		for ( auto iter = timer_sets_.begin();
				iter != timer_sets_.end(); ++iter ) {
			if ( *iter == ts ) {
				timer_sets_.erase(iter);
				break;
			}
		}
	}

	long long get_wait_ms()
	{
		long long min_wait_time = -1;

		for ( auto ts : timer_sets_ ) {
			long long wait_time = ts->get_wait_ms();
			if ( wait_time >= 0 ) {
				min_wait_time = min_wait_time < 0
					? wait_time
					: std::min(wait_time, min_wait_time);
			}
		}

		return min_wait_time;
	}

	void get_ready_internal_timer_ops(std::list<operation>& op_list)
	{
		for ( auto ts : timer_sets_ ) {
			ts->get_ready_internal_timer_ops(op_list);
		}
	}

	void get_all_timer_ops(std::list<operation>& op_list)
	{
		for ( auto ts : timer_sets_ ) {
			ts->cleanup_timers();
		}

		std::list<internal_timer*> timer_list;
		timer_datas_.get_live_objects(timer_list);

		for ( auto t : timer_list ) {
			op_list.push_back(std::move(t->timer_op_));
		}
	}

private:
	std::list<timer_set_base*> timer_sets_;
	object_pool<internal_timer> timer_datas_;
};

}	// namespace detail
}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_DETAIL_TIMER_SET_QUEUE_HPP__
