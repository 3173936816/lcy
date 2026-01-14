#ifndef __LCY_ASIO_STEADY_TIMER_H__
#define __LCY_ASIO_STEADY_TIMER_H__

#include <time.h>
#include <functional>

#include "lcy/asio/src/errinfo.h"
#include "lcy/asio/src/details/timer_service.h"

namespace lcy {
namespace asio {

class IOContext;

class SteadyTimer {
public:
	typedef time_t timeout_type;
	typedef std::function<void (errcode_type, timeout_type)> timer_op_type;

	SteadyTimer(IOContext& ioc, timeout_type timeout);
	~SteadyTimer();

	void cancel();
	void async_wait(timer_op_type timer_op);

	IOContext& context();

private:
	SteadyTimer(const SteadyTimer&);
	SteadyTimer& operator=(const SteadyTimer&);

private:
	typedef details::TimerService::timer_id_type timer_id_type;
	
	IOContext& ioc_;
	timeout_type timeout_;
	timer_id_type timer_id_;
	details::TimerService& timer_service_;
};

}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_STEADY_TIMER_H__
