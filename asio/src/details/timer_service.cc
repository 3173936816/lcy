#include "lcy/asio/src/details/timer_service.h"
#include "lcy/asio/src/details/reactor_service.h"
#include "lcy/asio/src/exception.h"

#include <set>
#include <mutex>
#include <vector>
#include <string.h>
#include <unistd.h>
#include <sys/timerfd.h>
#include <unordered_map>

namespace lcy {
namespace asio {
namespace details {

LCY_ASIO_DETAILS_SERVICEID_REGISTER(TimerService)

static int create_timerfd()
{
	int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
	if ( timerfd == -1 ) {
		throw LcyAsioException("timefd_create");
	}
	return timerfd;
}

static void destroy_timerfd(int fd)
{
	if ( fd != -1 ) {
		::close(fd);
	}
}

static time_t change_to_ms(const struct timespec& timeout)	
{
	return timeout.tv_sec * 1000 
		 + timeout.tv_nsec / 1000000;
}

time_t now_ms()
{
	struct timespec tim;
	::clock_gettime(CLOCK_MONOTONIC, &tim);

	return change_to_ms(tim);
}

static void update_timerfd(int fd, time_t absolute_time)
{
	struct itimerspec itim;
	::memset(&itim, 0x00, sizeof(itim));

	if ( absolute_time ) {
		time_t now = now_ms();

		if ( now >= absolute_time ) {		// Execute immediately  
			itim.it_value.tv_sec = 0;
			itim.it_value.tv_nsec = 1;		
		} else {
			time_t distance = absolute_time - now;
			
			itim.it_value.tv_sec = distance / 1000;
			itim.it_value.tv_nsec = (distance % 1000) * 1000000;
		}
	}

	::timerfd_settime(fd, 0, &itim, nullptr);
}

static void clear_timerfd(int fd)
{
	uint64_t u = 0;
	::read(fd, &u, sizeof(u));
}

///////////////////////////////////////////////////////////

class TimerIdAllocator {
public:
	typedef TimerService::timer_id_type timer_id_type;

	TimerIdAllocator();
	~TimerIdAllocator();

	int alloc();

private:
	std::mutex mutex_;
	timer_id_type alloc_id_;
};

//////////////////////////////////////////////////////////

class Timer {
public:
	typedef TimerService::timer_id_type timer_id_type;
	typedef TimerService::timer_op_type timer_op_type;

	Timer(timer_id_type timer_id,
		  timer_op_type timer_op,
		  const struct timespec& timeout);
	~Timer();

	timer_id_type timerId() const;
	time_t absoluteTime() const;
	void doTimerOperation() const;
	void cancelTimerOperation() const;

private:
	timer_id_type timer_id_;
	time_t absolute_time_;		// millisecond
	timer_op_type timer_op_;
};

//////////////////////////////////////////////////////////

Timer::Timer(timer_id_type timer_id,
	  		 timer_op_type timer_op,
	  		 const struct timespec& timeout) :
	timer_id_(timer_id),
	absolute_time_(now_ms() + change_to_ms(timeout)),
	timer_op_(std::move(timer_op))
{
}

Timer::~Timer()
{
}

Timer::timer_id_type Timer::timerId() const
{
	return timer_id_;
}

time_t Timer::absoluteTime() const
{
	return absolute_time_;
}

void Timer::doTimerOperation() const
{
	timer_op_(err::SUCCESS);
}

void Timer::cancelTimerOperation() const
{
	timer_op_(err::EOPCANCELED);
}

//////////////////////////////////////////////////////////

class TimerService::Impl {
public:
	Impl(ReactorService& reactor);
	~Impl();

	void registerTimer(timer_id_type& timer_id,
					   timer_op_type timer_op, 
					   const struct timespec& timeout);
	void cancelTimer(timer_id_type timer_id);
	void expireTimerExecute(errcode_type ec);
	
	ReactorService& reactor();

private:
	struct TimerComparator {
		bool operator()(const Timer* lhs, 
					  	const Timer* rhs) const;
	};

private:
	typedef int timerfd_type;
	typedef std::multiset<
				Timer*,
				TimerComparator
			> timer_mset_type;
	typedef std::unordered_map<
				timer_id_type,
				timer_mset_type::iterator
			> id_timer_umap_type;

	timerfd_type timerfd_;
	ReactorService& reactor_;
	timer_id_type id_allocator_;
	timer_mset_type timer_mset_;
	id_timer_umap_type id_timer_umap_;
};

/////////////////////////////////////////////////////////

bool TimerService::Impl::TimerComparator::operator()(const Timer* lhs, 
								 					 const Timer* rhs) const
{
	return lhs->absoluteTime() < rhs->absoluteTime();
}

////////////////////////////////////////////////////////

TimerService::Impl::Impl(ReactorService& reactor) :
	timerfd_(create_timerfd()),
	reactor_(reactor),
	id_allocator_(0)
{
	reactor_.registerReadOperation(timerfd_, std::bind(
		&TimerService::Impl::expireTimerExecute, this, std::placeholders::_1));
}

TimerService::Impl::~Impl()
{
	reactor_.removeAllOperations(timerfd_);
	destroy_timerfd(timerfd_);
	
	for ( auto timer : timer_mset_ ) {
		delete timer;
	}
}

void TimerService::Impl::registerTimer(timer_id_type& timer_id,
									   timer_op_type timer_op, 
				  					   const struct timespec& timeout)
{
	auto iter = id_timer_umap_.find(timer_id);	
	if ( iter != id_timer_umap_.end() ) {
		timer_op(err::EOPEXISTS);
		return;
	}

	timer_id = id_allocator_;
	if ( ++id_allocator_ < 0 ) {
		id_allocator_ = 0;
	}

	Timer* timer = new Timer(timer_id, std::move(timer_op), timeout);
	auto iter2 = timer_mset_.insert(timer);
	id_timer_umap_[timer_id] = iter2;

	if ( iter2 == timer_mset_.begin() ) {
		update_timerfd(timerfd_, timer->absoluteTime());
	}
}

void TimerService::Impl::cancelTimer(timer_id_type timer_id)
{
	auto iter = id_timer_umap_.find(timer_id);	
	if ( iter == id_timer_umap_.end() ) {
		return;
	}

	bool need_update = (iter->second == timer_mset_.begin());	

	const Timer* tmp_timer_op = *(iter->second);	// Save the pointer of the timer

	auto next_iter = timer_mset_.erase(iter->second);
	id_timer_umap_.erase(iter);

	if ( need_update ) {
		if ( next_iter == timer_mset_.end() ) {
			update_timerfd(timerfd_, 0);
		} else {
			update_timerfd(timerfd_, (*next_iter)->absoluteTime());
		}
	}

	tmp_timer_op->cancelTimerOperation();
	delete tmp_timer_op;		// delete the pointer of the timer
}

void TimerService::Impl::expireTimerExecute(errcode_type ec)
{
	if ( !ec ) {
		clear_timerfd(timerfd_);
	
		time_t now = now_ms();
		std::vector<Timer*> expire_timers;
	
		auto iter = timer_mset_.begin();
		for ( ; iter != timer_mset_.end(); ++iter ) {
			if ( (*iter)->absoluteTime() > now ) {
				break;
			}
	
			expire_timers.push_back(*iter);
			id_timer_umap_.erase((*iter)->timerId());
		}
		
		auto next_iter = timer_mset_.erase(timer_mset_.begin(), iter);
		if ( next_iter == timer_mset_.end() ) {
			update_timerfd(timerfd_, 0);
		} else {
			update_timerfd(timerfd_, (*next_iter)->absoluteTime());
		}
	
		for ( auto timer : expire_timers ) {
			timer->doTimerOperation();
			delete timer;
		}
	} else {
		 /*
 		 *notify :
 		 * 	This function never fails
 		 */ 
	}
}

ReactorService& TimerService::Impl::reactor()
{
	return reactor_;
}

//////////////////////////////////////////////////////////

TimerService::TimerService(ReactorService& reactor) :
	pImpl_(new Impl(reactor))
{
}

TimerService::~TimerService()
{
}

void TimerService::registerTimer(timer_id_type& timer_id,
								 timer_op_type timer_op, 
				  				 const struct timespec& timeout)
{
	pImpl_->registerTimer(timer_id, std::move(timer_op), timeout);
}

void TimerService::cancelTimer(timer_id_type timer_id)
{
	pImpl_->cancelTimer(timer_id);
}

ReactorService& TimerService::reactor()
{
	return pImpl_->reactor();
}

}	// namespace details
}	// namespace asio
}	// namespace lcy
