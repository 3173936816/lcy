#include "lcy/asio/src/steady_timer.h"
#include "lcy/asio/src/io_context.hpp"

#include <time.h>

namespace lcy {
namespace asio {

static void timer_op_wrap(SteadyTimer::timeout_type start,
						  errcode_type ec,
						  SteadyTimer::timer_op_type timer_op)
{
	if ( !ec ) {
		timer_op(ec, details::now_ms() - start);
	} else {
		/*
		*notify:
		*  This error was notified by the reactor ( mybe EOPEXISTS, EOPCANCELED )
		*  We just need to notify the caller.
		*/ 

		timer_op(ec, 0);
	}
}

////////////////////////////////////////////////////////////

SteadyTimer::SteadyTimer(IOContext& ioc, timeout_type timeout) :
	ioc_(ioc),
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
	timer_service_.cancelTimer(timer_id_);
	timer_id_ = -1;
}

void SteadyTimer::async_wait(timer_op_type timer_op)
{
	struct timespec t;
	t.tv_sec = timeout_ / 1000;
	t.tv_nsec = (timeout_ % 1000) * 1000000;

	timer_service_.registerTimer(timer_id_, std::bind(
		timer_op_wrap, details::now_ms(), std::placeholders::_1, 
			std::move(timer_op)), t);	
}

IOContext& SteadyTimer::context()
{
	return ioc_;
}

}	// namespace asio
}	// namespace lcy
