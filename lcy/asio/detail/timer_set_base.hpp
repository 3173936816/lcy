#ifndef __LCY_ASIO_DETAIL_TIMER_SET_BASE_HPP__
#define __LCY_ASIO_DETAIL_TIMER_SET_BASE_HPP__

#include <set>
#include <list>
#include <unordered_map>

#include "lcy/noncopyable.hpp"
#include "lcy/asio/detail/operation.hpp"

namespace lcy
{
namespace asio
{
namespace detail
{

class timer_set_base :
	private noncopyable
{
public:
	struct internal_timer
	{
		long long expiry_;
		operation timer_op_;
	};

	struct internal_timer_comparator
	{
		bool operator()(internal_timer* lhs, internal_timer* rhs) const
		{
			return lhs->expiry_ < rhs->expiry_;
		}
	};

public:
	virtual ~timer_set_base() {}

	bool has_internal_timer(internal_timer* t) const
	{
		auto iter = timer_set_index_.find(t);
		return iter != timer_set_index_.end();
	}

	bool insert_internal_timer(internal_timer* t, long long expiry, operation timer_op)
	{
		t->expiry_ = expiry;
		t->timer_op_ = std::move(timer_op);

		auto iter = timer_set_.insert(t);
		timer_set_index_.insert({t, iter});

		return timer_set_.begin() == iter;
	}

	bool remove_internal_timer(internal_timer* t)
	{
		bool is_earliest = false;

		auto iter = timer_set_index_.find(t);
		if ( iter != timer_set_index_.end() ) {
			if ( iter->second == timer_set_.begin() ) {
				is_earliest = true;
			}
			timer_set_.erase(iter->second);
			timer_set_index_.erase(iter);
		}

		return is_earliest;
	}

	void cleanup_timers()
	{
		timer_set_.clear();
		timer_set_index_.clear();
	}
	
	virtual long long get_wait_ms() = 0;
	virtual void get_ready_internal_timer_ops(std::list<operation>& op_list) = 0;

protected:
	std::multiset<
		internal_timer*,
		internal_timer_comparator
	> timer_set_;

	std::unordered_map<
		internal_timer*,
		std::multiset<
			internal_timer*,
			internal_timer_comparator
		>::iterator
	> timer_set_index_;
};

}	// namespace detail
}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_DETAIL_TIMER_SET_BASE_HPP__
