#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <sys/epoll.h> 
#include <sys/select.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <sys/signalfd.h>

#include "lcy/asio/detail/scheduler.hpp"
#include "lcy/asio/detail/throw_asio_exception.hpp"

namespace lcy
{
namespace asio
{
namespace detail
{

reactor::reactor(scheduler* sch) :
	scheduler_(sch),
	epoll_fd_(create_epollfd()),
	event_fd_(create_eventfd()),
	timer_fd_(create_timerfd()),
	signal_fd_(create_signalfd()),
	tsq_cond_mutex_(sch->concurrency_hint_ != 1),
	ssq_cond_mutex_(sch->concurrency_hint_ != 1),
	ds_cond_mutex_(sch->concurrency_hint_ != 1)
{
	epoll_event ep_event;
	ep_event.events = EPOLLIN | EPOLLET;

	ep_event.data.ptr = &event_fd_;
	if ( ::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, event_fd_, &ep_event) == -1 ) {
		lcy::system::error_code ec(errno);
		throw_asio_exception(ec, "reactor::reactor::epoll_ctl[event_fd_]");
	}

	ep_event.data.ptr = &timer_fd_;
	if ( ::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, timer_fd_, &ep_event) == -1 ) {
		lcy::system::error_code ec(errno);
		throw_asio_exception(ec, "reactor::reactor::epoll_ctl[timer_fd_]");
	}

	ep_event.data.ptr = &signal_fd_;
	if ( ::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, signal_fd_, &ep_event) == -1 ) {
		lcy::system::error_code ec(errno);
		throw_asio_exception(ec, "reactor::reactor::epoll_ctl[signal_fd_]");
	}
}

reactor::~reactor()
{
	epoll_event ep_event;
	::epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, signal_fd_, &ep_event);
	::epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, timer_fd_, &ep_event);
	::epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, event_fd_, &ep_event);

	destroy_fd(signal_fd_);
	destroy_fd(timer_fd_);
	destroy_fd(event_fd_);
	destroy_fd(epoll_fd_);
}

void reactor::wake_up()
{
	uint64_t u = 1;
	::write(event_fd_, &u, sizeof(u));
}

void reactor::run(int timeout, std::list<operation>& op_list)
{
	epoll_event ep_events[512];

	int event_count = ::epoll_wait(epoll_fd_, &ep_events[0], 512, timeout);
	for ( int i = 0; i < event_count; ++i ) {
		epoll_event* ev = &ep_events[i];

		if ( ev->data.ptr == &event_fd_ ) {
			handle_wake_up_events();
		}

		else if ( ev->data.ptr == &timer_fd_ ) {
			handle_timer_events(op_list);
		}

		else if ( ev->data.ptr == &signal_fd_ ) {
			handle_signal_events(op_list);
		}

		else {
			internal_descriptor_state* ds = 
				reinterpret_cast<internal_descriptor_state*>(ev->data.ptr);
			int happened_events = event_type::null;

			if ( ev->events & (EPOLLERR | EPOLLHUP) ) {
				happened_events |= event_type::error;
			}
			
			if ( ev->events & (EPOLLIN | EPOLLPRI) ) {
				happened_events |= event_type::read;
			}

			if ( ev->events & EPOLLOUT ) {
				happened_events |= event_type::write;
			}
			
			handle_descriptor_events(ds, happened_events, op_list);
		}
	}
}

void reactor::post_to_scheduler(operation op)
{
	scheduler_->post(std::move(op));
}

/*********************************************************
 *	About Socket Events
 *********************************************************/

reactor::internal_descriptor_state* reactor::alloc_internal_descriptor_state()
{
	std::lock_guard<conditionally_mutex> locker(ds_cond_mutex_);
	return descriptor_datas_.alloc(scheduler_->concurrency_hint_ != 1);
}

void reactor::free_internal_descriptor_state(internal_descriptor_state* ds)
{
	std::lock_guard<conditionally_mutex> locker(ds_cond_mutex_);
	descriptor_datas_.free(ds);
}

int reactor::register_internal_descriptor_state(int sockfd,
   internal_descriptor_state* ds)
{
	epoll_event ep_event;
	ep_event.events = EPOLLIN | EPOLLPRI | EPOLLET;
	ep_event.data.ptr = ds;

	if ( ::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, sockfd, &ep_event) == -1 ) {
		return errno;
	}

	std::lock_guard<conditionally_mutex> locker(ds->cond_mutex_);
	ds->registered_events_ = EPOLLIN | EPOLLPRI | EPOLLET;

	return 0;
}

void reactor::remove_internal_descriptor_state(int sockfd,
   internal_descriptor_state* ds)
{
	epoll_event ep_event;
	::epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, sockfd, &ep_event);

	std::lock_guard<conditionally_mutex> locker(ds->cond_mutex_);
	ds->registered_events_ = 0;
}

void reactor::start_op(int sockfd,
   internal_descriptor_state* ds, event_type et, operation op)
{
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(sockfd, &fds);
	timeval tv = { 0, 0 };

	int is_ready = 0;
	if ( et == event_type::read ) {
		is_ready = ::select(sockfd + 1, &fds, 0, 0, &tv);
	}
	if ( et == event_type::write ) {
		is_ready = ::select(sockfd + 1, 0, &fds, 0, &tv);
	}

	if ( is_ready > 0 ) {
		scheduler_->post(std::move(op));
		return;
	}

	std::lock_guard<conditionally_mutex> locker(ds->cond_mutex_);
	
	if ( et == event_type::write ) {
		if ( !(ds->registered_events_ & EPOLLOUT) ) {
			epoll_event ep_event;
			ep_event.events = ds->registered_events_ | EPOLLOUT;
			ep_event.data.ptr = ds;
			
			if ( ::epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, sockfd, &ep_event) == -1 ) {
				lcy::system::error_code ec(errno);
				op.set_errorc(ec);
				scheduler_->post(std::move(op));

				return;
			}
			
			ds->registered_events_ |= EPOLLOUT;
		}
		
		scheduler_->operation_increase();
		ds->write_op_ = std::move(op);
		ds->events_ |= event_type::write;
	}

	if ( et == event_type::read ) {
		scheduler_->operation_increase();
		ds->read_op_ = std::move(op);
		ds->events_ |= event_type::read;
	}	
}

void reactor::cancel_ops(int, internal_descriptor_state* ds, int ets)
{
	lcy::system::error_code ec =
		make_asio_error_code(lcy::asio::error::operation_abort);

	std::lock_guard<conditionally_mutex> locker(ds->cond_mutex_);

	if ( ets & event_type::read ) {
		if ( ds->events_ & event_type::read ) {
			ds->events_ &= ~event_type::read;
			ds->read_op_.set_errorc(ec);
			scheduler_->submit(std::move(ds->read_op_));
		}
	}

	if ( ets & event_type::write ) {
		if ( ds->events_ & event_type::write ) {
			ds->events_ &= ~event_type::write;
			ds->write_op_.set_errorc(ec);
			scheduler_->submit(std::move(ds->write_op_));
		}
	}
}

/*********************************************************
 *	About Timer Events
 *********************************************************/

void reactor::add_timer_set(timer_set_base* ts)
{
	std::lock_guard<conditionally_mutex> locker(tsq_cond_mutex_);
	ts_queue_.add_timer_set(ts);
}

void reactor::remove_timer_set(timer_set_base* ts)
{
	std::lock_guard<conditionally_mutex> locker(tsq_cond_mutex_);
	ts_queue_.remove_timer_set(ts);
}

reactor::internal_timer* reactor::alloc_internal_timer()
{
	std::lock_guard<conditionally_mutex> locker(tsq_cond_mutex_);
	return ts_queue_.alloc();
}

void reactor::free_internal_timer(internal_timer* t)
{
	std::lock_guard<conditionally_mutex> locker(tsq_cond_mutex_);
	ts_queue_.free(t);
}

void reactor::schedule_timer(timer_set_base* ts,
	internal_timer* t, long long expiry, operation timer_op)
{
	std::lock_guard<conditionally_mutex> locker(tsq_cond_mutex_);

	scheduler_->operation_increase();
	bool need_update = ts->insert_internal_timer(t, expiry, std::move(timer_op));
	if ( need_update ) {
		long long wait_ms = ts_queue_.get_wait_ms();
		update_timerfd(wait_ms);
	}
}

void reactor::cancel_timer(timer_set_base* ts, internal_timer* t)
{
	std::lock_guard<conditionally_mutex> locker(tsq_cond_mutex_);
	
	if ( ts->has_internal_timer(t) ) {
		bool need_update = ts->remove_internal_timer(t);
		if ( need_update ) {
			long long wait_ms = ts_queue_.get_wait_ms();
			update_timerfd(wait_ms);
		}

		lcy::system::error_code ec =
			make_asio_error_code(lcy::asio::error::operation_abort);
		t->timer_op_.set_errorc(ec);
		scheduler_->submit(std::move(t->timer_op_));
	}
}

/*********************************************************
 *	About Signal Events
 *********************************************************/

reactor::internal_signal_set* reactor::alloc_internal_signal_set()
{
	std::lock_guard<conditionally_mutex> locker(ssq_cond_mutex_);
	return ss_queue_.alloc();
}

void reactor::free_internal_signal_set(internal_signal_set* s)
{
	std::lock_guard<conditionally_mutex> locker(ssq_cond_mutex_);
	return ss_queue_.free(s);
}

void reactor::capture_signal_set(internal_signal_set* s,
   uint64_t sigmask, operation signal_op)
{
	std::lock_guard<conditionally_mutex> locker(ssq_cond_mutex_);
	
	scheduler_->operation_increase();	
	bool need_update = ss_queue_.add_internal_signal_set(s,
		sigmask, std::move(signal_op));
		
	if ( need_update ) {
		std::list<int> signum_list;
		ss_queue_.get_signal_table(signum_list);

		update_signalfd(std::move(signum_list));
	}
}

void reactor::cancel_signal_set(internal_signal_set* s)
{
	std::lock_guard<conditionally_mutex> locker(ssq_cond_mutex_);
	if ( ss_queue_.has_internal_signal_set(s) ) {
		ss_queue_.remove_internal_signal_set(s);

		lcy::system::error_code ec =
			make_asio_error_code(lcy::asio::error::operation_abort);
		s->signal_op_.set_errorc(ec);
		scheduler_->submit(std::move(s->signal_op_));
	}
}

/*********************************************************
 *	About Helper Functions
 *********************************************************/

int reactor::create_epollfd()
{
	int fd = ::epoll_create1(EPOLL_CLOEXEC);
	if ( fd == -1 ) {
		lcy::system::error_code ec(errno);
		throw_asio_exception(ec, "reactor::create_epollfd::epoll_create1");
	}

	return fd;
}

int reactor::create_eventfd()
{
	int fd = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
	if ( fd == -1 ) {
		lcy::system::error_code ec(errno);
		throw_asio_exception(ec, "reactor::create_eventfd::eventfd");
	}

	return fd;
}

int reactor::create_timerfd()
{
	int fd = ::timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
	if ( fd == -1 ) {
		lcy::system::error_code ec(errno);
		throw_asio_exception(ec, "reactor::create_timerfd::timerfd_create");
	}
	
	return fd;
}

int reactor::create_signalfd()
{
	sigset_t mask;
	::sigemptyset(&mask);

	int fd = ::signalfd(-1, &mask, SFD_CLOEXEC | SFD_NONBLOCK);
	if ( fd == -1 ) {
		lcy::system::error_code ec(errno);
		throw_asio_exception(ec, "reactor::create_signalfd::signalfd");
	}
	
	return fd;
}

void reactor::destroy_fd(int fd)
{
	if ( fd != -1 ) {
		::close(fd);
	}
}

void reactor::update_timerfd(long long wait_ms)
{
	itimerspec itspec;
	if ( wait_ms == -1 ) {
		itspec = { {0, 0}, {0, 0} };
	} else if ( wait_ms == 0 ) {
		itspec = { {0, 0}, {0, 1} };
	} else {
		itspec = { {0, 0}, { wait_ms / 1000, (wait_ms % 1000) * 1000000 } };
	}

	::timerfd_settime(timer_fd_, 0, &itspec, 0);
}

void reactor::update_signalfd(std::list<int> signum_list)
{
	sigset_t mask;
	::sigemptyset(&mask);

	for ( auto signum : signum_list ) {
		::sigaddset(&mask, signum);
	}

	::signalfd(signal_fd_, &mask, 0);
}

void reactor::handle_wake_up_events()
{
	uint64_t u = 0;
	::read(event_fd_, &u, sizeof(u));
}

void reactor::handle_timer_events(std::list<operation>& op_list)
{
	std::lock_guard<conditionally_mutex> locker(tsq_cond_mutex_);

	ts_queue_.get_ready_internal_timer_ops(op_list);
	long long wait_ms = ts_queue_.get_wait_ms();

	update_timerfd(wait_ms);
}

void reactor::handle_signal_events(std::list<operation>& op_list)
{
	std::lock_guard<conditionally_mutex> locker(ssq_cond_mutex_);
	
	signalfd_siginfo siginfo;
	while ( ::read(signal_fd_, &siginfo, sizeof(siginfo)) == sizeof(siginfo) ) {
		ss_queue_.get_ready_internal_signal_set_ops(siginfo.ssi_signo, op_list);
	}
}

void reactor::handle_descriptor_events(internal_descriptor_state* ds,
	int happened_events, std::list<operation>& op_list)
{
	std::lock_guard<conditionally_mutex> locker(ds->cond_mutex_);

	if ( happened_events & event_type::error ) {
		lcy::system::error_code ec(lcy::system::error::bad_file_descriptor);

		if ( ds->events_ & event_type::read ) {
			ds->events_ &= ~event_type::read;
			ds->read_op_.set_errorc(ec);
			op_list.push_back(std::move(ds->read_op_));
		}

		if ( ds->events_ & event_type::write ) {
			ds->events_ &= ~event_type::write;
			ds->write_op_.set_errorc(ec);
			op_list.push_back(std::move(ds->write_op_));
		}
	} else {
		if ( happened_events & event_type::read ) {
			if ( ds->events_ & event_type::read ) {
				ds->events_ &= ~event_type::read;
				op_list.push_back(std::move(ds->read_op_));
			}
		}

		if ( happened_events & event_type::write ) {
			if ( ds->events_ & event_type::write ) {
				ds->events_ &= ~event_type::write;
				op_list.push_back(std::move(ds->write_op_));
			}
		}
	}
}

void reactor::cleanup()
{
	std::list<operation> abandon_ops;

	std::lock_guard<conditionally_mutex> ts_locker(tsq_cond_mutex_);
	ts_queue_.get_all_timer_ops(abandon_ops);

	std::lock_guard<conditionally_mutex> ss_locker(ssq_cond_mutex_);
	ss_queue_.get_all_signal_set_ops(abandon_ops);

	std::lock_guard<conditionally_mutex> ds_locker(ds_cond_mutex_);
	std::list<internal_descriptor_state*> descriptor_state_list;
	descriptor_datas_.get_live_objects(descriptor_state_list);
	for ( auto ds : descriptor_state_list ) {
		std::lock_guard<conditionally_mutex> ss_locker(ds->cond_mutex_);
		if ( ds->events_ & event_type::read ) {
			abandon_ops.push_back(std::move(ds->read_op_));
		}
		if ( ds->events_ & event_type::write ) {
			abandon_ops.push_back(std::move(ds->write_op_));
		}
		ds->events_ = event_type::null;
	}

	scheduler_->abandon_operations(abandon_ops);
}

}	// namespace detail
}	// namespace asio
}	// namespace lcy
