#include "lcy/asio/detail/reactor.hpp"

namespace lcy
{
namespace asio
{
namespace detail
{

scheduler::scheduler(std::size_t concurrency_hint) :
	concurrency_hint_(concurrency_hint),
	stopped_(false),
	block_on_reactor_(false),
	pending_operations_(0),
	condvar_block_threads_(0),
	reactor_mutex_(concurrency_hint_ != 1),	
	reactor_(0),
	op_queue_mutex_(concurrency_hint_ != 1)
{
}

scheduler::~scheduler()
{
	if ( reactor_ ) {
		delete reactor_;
	}
}

std::size_t scheduler::run()
{
	std::size_t n = 0;
	
	if ( pending_operations_ == 0 ) {
		stop();
		return n;
	}

	while ( !stopped_ ) {
		op_queue_mutex_.lock();

		if ( !op_queue_.empty() ) {
			scheduler_operation scheduler_op = std::move(op_queue_.front());
			op_queue_.pop_front();

			bool has_more_op = !op_queue_.empty();

			op_queue_mutex_.unlock();

			if ( concurrency_hint_ != 1 && has_more_op ) {
				wake_up_one_thread();
			}
			
			if ( scheduler_op.event_token_ ) {
				int timeout = has_more_op ? 0 : -1;
				if ( timeout == -1 ) {
					block_on_reactor_ = true;
				}
				
				std::list<operation> private_op_queue;
				reactor_->run(timeout, private_op_queue);

				block_on_reactor_ = false;
				submit_private_op_queue(std::move(private_op_queue));
			} else {
				++n;
				scheduler_op();
				operation_decrease();
			}
		} else {
			condvar_.wait(op_queue_mutex_, [this](){
				return !op_queue_.empty() || stopped_;
			});
		}
	}

	return n;
}

void scheduler::stop()
{
	if ( !stopped_ ) {
		stop_all_threads();
	}
}

void scheduler::post(operation op)
{
	operation_increase();
	submit(std::move(op));
}

void scheduler::submit(operation op)
{
	scheduler_operation scheduler_op(std::move(op));

	std::lock_guard<conditionally_mutex> locker(op_queue_mutex_);
	op_queue_.push_back(std::move(scheduler_op));

	wake_up_one_thread();
}

void scheduler::operation_increase()
{
	++pending_operations_;
}

void scheduler::operation_decrease()
{
	--pending_operations_;

	if ( pending_operations_ == 0 ) {
		stop();
	}
}

reactor& scheduler::get_reactor()
{
	std::lock_guard<conditionally_mutex> locker(reactor_mutex_);
	if ( !reactor_ ) {
		init_reactor();
	}

	return *reactor_;
}

void scheduler::init_reactor()
{
	reactor_ = new reactor(this);
	
	std::lock_guard<conditionally_mutex> locker(op_queue_mutex_);
	op_queue_.push_back(scheduler_operation());
}

void scheduler::wake_up_one_thread()
{
	if ( condvar_.notify_one() ) {
		return;
	}
	
	std::lock_guard<conditionally_mutex> locker(reactor_mutex_);
	if ( reactor_ && block_on_reactor_ ) {
		reactor_->wake_up();
		block_on_reactor_ = false;
	}
}

void scheduler::stop_all_threads()
{
	stopped_ = true;

	condvar_.notify_all();
	std::lock_guard<conditionally_mutex> locker(reactor_mutex_);
	if ( reactor_ ) {
		reactor_->wake_up();
	}
}

void scheduler::submit_private_op_queue(std::list<operation> op_list)
{
	std::list<scheduler_operation> scheduler_op_list;
	for ( auto& op : op_list ) {
		scheduler_operation scheduler_op(std::move(op));
		scheduler_op_list.push_back(std::move(scheduler_op));
	}
	scheduler_op_list.push_back(scheduler_operation());

	std::lock_guard<conditionally_mutex> locker(op_queue_mutex_);
	op_queue_.splice(op_queue_.end(), scheduler_op_list);

	condvar_.notify_one();
}

void scheduler::cleanup()
{
	std::lock_guard<conditionally_mutex> locker(reactor_mutex_);
	if ( reactor_ ) {
		reactor_->cleanup();
	}

	std::lock_guard<conditionally_mutex> locker(op_queue_mutex_);
	op_queue_.clear();
}

void scheduler::abandon_operations(std::list<operation>& op_list)
{
	std::list<operation> abandon_list = std::move(op_list);
}

}	// namespace detail
}	// namespace asio
}	// namespace lcy
