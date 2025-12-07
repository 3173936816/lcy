#ifndef __LCY_ASIO_STEADY_TIMER_H__
#define __LCY_ASIO_STEADY_TIMER_H__

#include <time.h>
#include <functional>

namespace lcy {
namespace asio {
namespace details {
	class TimerService;
}	// namespace details

class IOContext;

class SteadyTimer {
public:
	typedef time_t timeout_type;
	typedef std::function<void (int, timeout_type)> timer_callback_type;

	SteadyTimer(IOContext& ioc, timeout_type timeout);
	~SteadyTimer();

	void cancel();
	void async_wait(timer_callback_type timer_cb);

private:
	SteadyTimer(const SteadyTimer&);
	SteadyTimer& operator=(const SteadyTimer&);

private:
	typedef int timer_id_type;
	
	timeout_type timeout_;
	timer_id_type timer_id_;
	details::TimerService& timer_service_;
};

}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_STEADY_TIMER_H__
