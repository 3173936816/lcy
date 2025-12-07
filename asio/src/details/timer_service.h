#ifndef __LCY_ASIO_DETAILS_TIMER_SERVICE_H__
#define __LCY_ASIO_DETAILS_TIMER_SERVICE_H__

#include "asio/src/details/service.hpp"
#include "asio/src/details/channel.h"

#include <time.h>
#include <memory>
#include <functional>

namespace lcy {
namespace asio {
namespace details {

class ReactorService;

class TimerService :
	public Service
{
public:
	typedef std::function<void ()> timer_callback_type;

	TimerService(ReactorService& reactor);
	~TimerService();

	int registerTimer(timer_callback_type timer_cb, 
					  const struct timespec& timeout);
	int cancelTimer(int timer_id);
	
	ReactorService& reactor();

private:
	TimerService(const TimerService& timer_service);
	TimerService& operator=(const TimerService& timer_service);

private:
	class Impl;
	std::unique_ptr<Impl> pImpl_;
};

time_t now_ms();

LCY_ASIO_DETAILS_SERVICEID_REGISTER_EXTERN(TimerService)

}	// namespace details
}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_DETAILS_TIMER_SERVICE_H__
