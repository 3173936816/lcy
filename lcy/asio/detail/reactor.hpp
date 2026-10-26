#ifndef __LCY_ASIO_DETAIL_REACTOR_HPP__
#define __LCY_ASIO_DETAIL_REACTOR_HPP__

#include "lcy/noncopyable.hpp"
#include "lcy/object_pool.hpp"
#include "lcy/asio/detail/error.hpp"
#include "lcy/asio/detail/operation.hpp"
#include "lcy/asio/detail/timer_set_queue.hpp"
#include "lcy/asio/detail/signal_set_queue.hpp"
#include "lcy/asio/detail/conditionally_mutex.hpp"

namespace lcy
{
namespace asio
{
namespace detail
{

class scheduler;

class reactor :
	private noncopyable
{
public:
	enum event_type
	{
		null = 0x0000,
		read = 0x0001,
		write = 0x0010,
		error = 0x0100
	};

	struct internal_descriptor_state
	{
		conditionally_mutex cond_mutex_;

		operation read_op_;
		operation write_op_;
		int events_;
		uint32_t registered_events_;

		explicit internal_descriptor_state(bool enable_lock) :
			cond_mutex_(enable_lock),
			events_(event_type::null),
			registered_events_(0) {}
	};

public:
	typedef timer_set_base::internal_timer internal_timer;
	typedef signal_set_queue::internal_signal_set internal_signal_set;

	inline explicit reactor(scheduler* sch);
	inline ~reactor();

	inline void wake_up();
	inline void run(int timeout, std::list<operation>& ops);

	inline void post_to_scheduler(operation op);


	inline internal_descriptor_state* alloc_internal_descriptor_state();
	inline void free_internal_descriptor_state(internal_descriptor_state* ds);

	inline int register_internal_descriptor_state(int sockfd,
		internal_descriptor_state* ds);
	inline void remove_internal_descriptor_state(int sockfd,
		internal_descriptor_state* ds);
	inline void start_op(int sockfd,
		internal_descriptor_state* ds, event_type et,
		operation op, bool try_speculative);
	inline void cancel_ops(int sockfd, internal_descriptor_state* ds, int ets);


	inline void add_timer_set(timer_set_base* ts);
	inline void remove_timer_set(timer_set_base* ts);

	inline internal_timer* alloc_internal_timer();
	inline void free_internal_timer(internal_timer* t);

	inline void schedule_timer(timer_set_base* ts,
		internal_timer* t, long long expiry, operation timer_op);
	inline void cancel_timer(timer_set_base* ts, internal_timer* t);


	inline internal_signal_set* alloc_internal_signal_set();
	inline void free_internal_signal_set(internal_signal_set* s);

	inline void capture_signal_set(internal_signal_set* s,
		uint64_t sigmask, operation signal_op);
	inline void cancel_signal_set(internal_signal_set* s);


	inline void cleanup();

private:
	inline int create_epollfd();
	inline int create_eventfd();
	inline int create_timerfd();
	inline int create_signalfd();
	inline void destroy_fd(int fd);

	void update_timerfd(long long timeout);
	void update_signalfd(const std::list<int>& signum_list);

	inline void handle_wake_up_events();
	inline void handle_timer_events(std::list<operation>& op_list);
	inline void handle_signal_events(std::list<operation>& op_list);
	inline void handle_descriptor_events(internal_descriptor_state* ds,
		int happened_events , std::list<operation>& op_list);

private:
	scheduler* scheduler_;

	int epoll_fd_;
	int event_fd_;
	int timer_fd_;
	int signal_fd_;

	conditionally_mutex tsq_cond_mutex_;
	timer_set_queue ts_queue_;

	conditionally_mutex ssq_cond_mutex_;
	signal_set_queue ss_queue_;

	conditionally_mutex ds_cond_mutex_;
	object_pool<internal_descriptor_state> descriptor_datas_;
};

}	// namespace detail
}	// namespace asio
}	// namespace lcy

#include "lcy/asio/detail/impl/reactor.ipp"

#endif	// __LCY_ASIO_DETAIL_REACTOR_HPP__
