#ifndef __LCY_ASIO_DETAIL_SCHEDULER_HPP__
#define __LCY_ASIO_DETAIL_SCHEDULER_HPP__

#include <list>
#include <atomic>

#include "lcy/noncopyable.hpp"
#include "lcy/asio/detail/operation.hpp"
#include "lcy/asio/detail/conditionally_mutex.hpp"
#include "lcy/asio/detail/conditionally_condition_variable.hpp"

namespace lcy
{
namespace asio
{
namespace detail
{

class reactor;

class scheduler :
	private noncopyable
{
private:
	struct scheduler_operation
	{
		operation op_;
		bool event_token_;

		scheduler_operation() :
			event_token_(true) {}

		scheduler_operation(operation op) :
			op_(std::move(op)), event_token_(false) {}

		void operator()() const
		{
			op_();
		}
	};

public:
	inline explicit scheduler(std::size_t concurrency_hint);
	inline ~scheduler();

	inline std::size_t run();
	inline void stop();

	inline void post(operation op);
	inline void submit(operation op);

	inline void operation_increase();
	inline void operation_decrease();

	inline reactor& get_reactor();

	const std::size_t concurrency_hint_;
private:
	inline void init_reactor();
	inline void wake_up_one_thread();
	inline void stop_all_threads();
	inline void submit_private_op_queue(std::list<operation> op_list);

private:
	std::atomic<std::size_t> stopped_;
	std::atomic<bool> block_on_reactor_;
	std::atomic<std::size_t> pending_operations_;
	std::atomic<std::size_t> condvar_block_threads_;

	conditionally_mutex reactor_mutex_;
	reactor* reactor_;

	conditionally_mutex op_queue_mutex_;
	conditionally_condition_variable condvar_;
	std::list<scheduler_operation> op_queue_;
};

}	// namespace detail
}	// namespace asio
}	// namespace lcy

#include "lcy/asio/detail/impl/scheduler.ipp"

#endif	// __LCY_ASIO_DETAIL_SCHEDULER_HPP__
