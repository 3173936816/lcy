#include "asio/src/details/timer_service.h"
#include "asio/src/details/reactor_service.h"
#include "asio/src/details/channel.h"
#include "asio/src/exception.h"

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
	if ( fd != -1 && ::close(fd) ) {
		throw LcyAsioException("timefd close");
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
	if ( ::clock_gettime(CLOCK_MONOTONIC, &tim) ) {
		throw LcyAsioException("clock_gettime");
	}

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

	if ( ::timerfd_settime(fd, 0, &itim, nullptr) ) {
		throw LcyAsioException("timefd_settime");
	}
}

static void clear_timerfd(int fd)
{
	uint64_t u = 0;
	if ( ::read(fd, &u, sizeof(u)) != sizeof(u) ) {
		if ( errno != EAGAIN ) {
			throw LcyAsioException("timefd read");
		}
	}
}

///////////////////////////////////////////////////////////

class TimerIdAllocator {
public:
	TimerIdAllocator();
	~TimerIdAllocator();

	int alloc();

private:
	std::mutex mutex_;
	int alloc_id_;
};

//////////////////////////////////////////////////////////

TimerIdAllocator::TimerIdAllocator() : 
	alloc_id_(-1) 
{
}

TimerIdAllocator::~TimerIdAllocator() 
{
}

int TimerIdAllocator::alloc()
{
	std::lock_guard<std::mutex> locker(mutex_);

	if ( ++alloc_id_ < 0 ) {
		alloc_id_ = 0;
	}
	
	return alloc_id_;
}

static TimerIdAllocator s_timerid_allocator;

///////////////////////////////////////////////////////////

class Timer {
public:
	typedef TimerService::timer_callback_type timer_callback_type;

	Timer(int timer_id,
		  timer_callback_type timer_cb,
		  const struct timespec& timeout);
	~Timer();

	int timerId() const;
	time_t absoluteTime() const;
	timer_callback_type timerCallback() const;

private:
	int timer_id_;
	time_t absolute_time_;		// millisecond
	timer_callback_type timer_cb_;
};

//////////////////////////////////////////////////////////

Timer::Timer(int timer_id,
	  		 timer_callback_type timer_cb,
	  		 const struct timespec& timeout) :
	timer_id_(timer_id),
	absolute_time_(now_ms() + change_to_ms(timeout)),
	timer_cb_(std::move(timer_cb))
{
}

Timer::~Timer()
{
}

int Timer::timerId() const
{
	return timer_id_;
}

time_t Timer::absoluteTime() const
{
	return absolute_time_;
}

Timer::timer_callback_type Timer::timerCallback() const
{
	return timer_cb_;
}

//////////////////////////////////////////////////////////

class TimerService::Impl {
public:
	Impl(ReactorService& reactor);
	~Impl();

	int registerTimer(timer_callback_type timer_cb, 
					  const struct timespec& timeout);
	int cancelTimer(int timer_id);
	void expireTimerExecute();
	
	ReactorService& reactor();

private:
	struct TimerComparator {
		bool operator()(const Timer* lhs, 
					  	const Timer* rhs) const;
	};

private:
	typedef std::multiset<
				Timer*,
				TimerComparator
			> timer_mset_type;
	typedef std::unordered_map<
				int, 
				timer_mset_type::iterator
			> id_timer_umap_type;

	Channel channel_;
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
	channel_(create_timerfd(), reactor)
{
	channel_.registerReadCallback(std::bind(		// FIXME : Check return value
		&TimerService::Impl::expireTimerExecute, this));
}

TimerService::Impl::~Impl()
{
	channel_.cancelReading();		// FIXME : Check return value
	destroy_timerfd(channel_.fd());
	
	for ( auto timer : timer_mset_ ) {
		delete timer;
	}
}

int TimerService::Impl::registerTimer(timer_callback_type timer_cb, 
				  					  const struct timespec& timeout)
{
	int timer_id = s_timerid_allocator.alloc();

	auto iter = id_timer_umap_.find(timer_id);	
	if ( iter != id_timer_umap_.end() ) {
		return -1;
	}

	Timer* timer = new Timer(timer_id, std::move(timer_cb), timeout);
	auto iter2 = timer_mset_.insert(timer);
	id_timer_umap_[timer_id] = iter2;

	if ( iter2 == timer_mset_.begin() ) {
		update_timerfd(channel_.fd(), timer->absoluteTime());
	}

	return timer_id;
}

int TimerService::Impl::cancelTimer(int timer_id)
{
	auto iter = id_timer_umap_.find(timer_id);	
	if ( iter == id_timer_umap_.end() ) {
		return -1;
	}

	bool need_update = (iter->second == timer_mset_.begin());
	
	delete *(iter->second);

	auto next_iter = timer_mset_.erase(iter->second);
	id_timer_umap_.erase(iter);

	if ( need_update ) {
		if ( next_iter == timer_mset_.end() )
			update_timerfd(channel_.fd(), 0);
		else
			update_timerfd(channel_.fd(), (*next_iter)->absoluteTime());
	}

	return 0;
}

void TimerService::Impl::expireTimerExecute()
{
	clear_timerfd(channel_.fd());

	time_t now = now_ms();
	std::vector<Timer*> expire_timers;

	auto iter = timer_mset_.begin();
	for ( ; iter != timer_mset_.end(); ++iter ) {
		if ( (*iter)->absoluteTime() > now )
			break;

		expire_timers.push_back(*iter);
		id_timer_umap_.erase((*iter)->timerId());
	}
	
	auto next_iter = timer_mset_.erase(timer_mset_.begin(), iter);
	if ( next_iter == timer_mset_.end() )
		update_timerfd(channel_.fd(), 0);
	else
		update_timerfd(channel_.fd(), (*next_iter)->absoluteTime());

	try {
		for ( auto timer : expire_timers ) {
			(timer->timerCallback())();
		}
		for ( auto timer : expire_timers ) {
			delete timer;
		}
	} catch (...) {
		for ( auto timer : expire_timers ) {
			delete timer;
		}
		throw;
	}
}

ReactorService& TimerService::Impl::reactor()
{
	return channel_.reactor();
}

//////////////////////////////////////////////////////////
TimerService::TimerService(ReactorService& reactor) :
	pImpl_(new Impl(reactor))
{
}

TimerService::~TimerService()
{
}

int TimerService::registerTimer(timer_callback_type timer_cb, 
				  				const struct timespec& timeout)
{
	return pImpl_->registerTimer(std::move(timer_cb), timeout);
}

int TimerService::cancelTimer(int timer_id)
{
	return pImpl_->cancelTimer(timer_id);
}

ReactorService& TimerService::reactor()
{
	return pImpl_->reactor();
}

}	// namespace details
}	// namespace asio
}	// namespace lcy
