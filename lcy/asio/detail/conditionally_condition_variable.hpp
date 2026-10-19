#ifndef __LCY_ASIO_DETAIL_CONDITIONALLY_CONDITION_VARIABLE_HPP__
#define __LCY_ASIO_DETAIL_CONDITIONALLY_CONDITION_VARIABLE_HPP__

#include <unistd.h>

#include <climits>
#include <cstdint>
#include <functional>
#include <condition_variable>

#include "lcy/noncopyable.hpp"
#include "lcy/asio/detail/conditionally_mutex.hpp"

namespace lcy
{
namespace asio
{
namespace detail
{

class conditionally_condition_variable :
	private noncopyable
{
public:
	typedef std::function<bool()> predicate_func_type;

	conditionally_condition_variable() :
		block_threads_(0) {}

	void wait(conditionally_mutex& cond_mutex, predicate_func_type predicate)
	{
		if ( cond_mutex.enable_lock() ) {
			std::unique_lock<std::mutex> locker(cond_mutex.get_mutex(), std::adopt_lock);
			
			++block_threads_;
			cond_.wait(locker, predicate);
			--block_threads_;
		} else {
			::sleep(UINT_MAX);
		}
	}

	bool notify_one()
	{
		bool has_thread_resume = false;
		if ( block_threads_ ) {
			cond_.notify_one();
			has_thread_resume = true;
		}

		return has_thread_resume;
	}

	void notify_all()
	{
		cond_.notify_all();		
	}

private:
	std::size_t block_threads_;
	std::condition_variable cond_;
};

}	// namespace detail
}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_DETAIL_CONDITIONALLY_CONDITION_VARIABLE_HPP__
