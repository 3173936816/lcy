#ifndef __LCY_ASIO_DETAIL_SIGNAL_SET_QUEUE_HPP__
#define __LCY_ASIO_DETAIL_SIGNAL_SET_QUEUE_HPP__

#include <list>
#include <cstdint>
#include <unordered_set>

#include "lcy/noncopyable.hpp"
#include "lcy/object_pool.hpp"
#include "lcy/asio/detail/operation.hpp"

namespace lcy
{
namespace asio
{
namespace detail
{

class signal_set_queue :
	private noncopyable
{
public:
	struct internal_signal_set
	{
		uint64_t sigmask_;
		operation signal_op_;
	};

public:
	signal_set_queue() :
		sigmask_table_(0) {}

	internal_signal_set* alloc()
	{
		return signal_set_datas_.alloc();
	}

	void free(internal_signal_set* s)
	{
		signal_set_datas_.free(s);
	}

	bool has_internal_signal_set(internal_signal_set* s) const
	{
		auto iter = signal_set_index_.find(s);
		return iter != signal_set_index_.end();
	}

	bool add_internal_signal_set(internal_signal_set* s,
			uint64_t sigmask, operation signal_op)
	{
		s->sigmask_ = sigmask;
		s->signal_op_ = std::move(signal_op);

		signal_sets_.push_front(s);
		signal_set_index_.insert({s, signal_sets_.begin()});

		uint64_t tmp_table = sigmask_table_;
		sigmask_table_ |= sigmask;

		return tmp_table != sigmask_table_;
	}

	void remove_internal_signal_set(internal_signal_set* s)
	{
		auto iter = signal_set_index_.find(s);
		if ( iter != signal_set_index_.end() ) {
			signal_sets_.erase(iter->second);
			signal_set_index_.erase(iter);
		}
	}

	void get_ready_internal_signal_set_ops(int target_signum,
		std::list<operation>& op_list)
	{
		for ( auto iter = signal_sets_.begin(); iter != signal_sets_.end(); ) {
			internal_signal_set* s = *iter;
			bool has_target_signum = s->sigmask_ 
				& (static_cast<uint64_t>(1) << (target_signum - 1));

			if ( has_target_signum ) {
				s->signal_op_.set_signum(target_signum);
				op_list.push_back(std::move(s->signal_op_));

				iter = signal_sets_.erase(iter);
				signal_set_index_.erase(s);
			} else {
				++iter;
			}
		}
	}

	void get_signal_table(std::list<int>& signum_list) const
	{
		for ( auto table = sigmask_table_; table > 0; table &= table - 1 ) {
			int signum = __builtin_ctzll(table) + 1;
			signum_list.push_back(signum);
		}
	}

private:
	std::list<internal_signal_set*> signal_sets_;
	std::unordered_map<
		internal_signal_set*,
		std::list<
			internal_signal_set*
		>::iterator
	> signal_set_index_;

	uint64_t sigmask_table_;
	object_pool<internal_signal_set> signal_set_datas_;
};

}	// namespace detail
}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_DETAIL_SIGNAL_SET_QUEUE_HPP__
