#include "asio/src/steady_timer.h"
#include "asio/src/io_context.hpp"
#include "asio/src/errinfo.h"

#include <time.h>

namespace lcy {
namespace asio {

static void timer_cb_wrap(SteadyTimer::timeout_type start,
						  SteadyTimer::timer_callback_type timer_cb)
{
	SteadyTimer::timeout_type distance_ms = details::now_ms() - start;
	if ( distance_ms < 0 ) {
		timer_cb(err::ESYSTEM, distance_ms);
	}

	timer_cb(err::SUCCESS, distance_ms);
}

////////////////////////////////////////////////////////////

SteadyTimer::SteadyTimer(IOContext& ioc, timeout_type timeout) :
	timeout_(timeout),
	timer_id_(-1),
	timer_service_(use_service<details::TimerService>(ioc))
{
}

SteadyTimer::~SteadyTimer()
{
	cancel();
}

void SteadyTimer::cancel()
{
	if ( timer_id_ != -1 ) {
		timer_service_.cancelTimer(timer_id_);
		timer_id_ = -1;
	}
}

void SteadyTimer::async_wait(timer_callback_type timer_cb)
{
	cancel();

	struct timespec t;
	t.tv_sec = timeout_ / 1000;
	t.tv_nsec = (timeout_ % 1000) * 1000000;

	timer_id_ = timer_service_.registerTimer(std::bind(
			timer_cb_wrap, details::now_ms(), timer_cb), t);	// timer_cb can not be moved
	if ( timer_id_ == -1 ) {							
		timer_cb(err::ESYSTEM, 0);								// this need to be used
	}
}

}	// namespace asio
}	// namespace lcy
