#ifndef __LCY_ASIO_DETAILS_TIMER_SERVICE_H__
#define __LCY_ASIO_DETAILS_TIMER_SERVICE_H__

#include <time.h>
#include <memory>
#include <functional>

#include "lcy/asio/src/errinfo.h"
#include "lcy/asio/src/details/service.hpp"

namespace lcy {
namespace asio {
namespace details {

class ReactorService;

class TimerService :
	public Service
{
public:
	typedef int64_t timer_id_type;
	typedef std::function<void (errcode_type)> timer_op_type;

	TimerService(ReactorService& reactor);
	~TimerService();

	void registerTimer(timer_id_type& timer_id,
					   timer_op_type timer_op, 
					   const struct timespec& timeout);
	void cancelTimer(timer_id_type timer_id);
	
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
