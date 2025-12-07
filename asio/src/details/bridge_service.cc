#include "asio/src/details/bridge_service.hpp"
#include "asio/src/details/reactor_service.h"
#include "asio/src/exception.h"

#include <sys/eventfd.h>

namespace lcy {
namespace asio {
namespace details {

LCY_ASIO_DETAILS_SERVICEID_REGISTER(BridgeService)

static int create_eventfd()
{
	int eventfdval = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
	if ( eventfdval == -1 ) {
		throw LcyAsioException("eventfd");
	}
	return eventfdval;
}

static void destroy_eventfd(int fd)
{
	if ( fd != -1 && ::close(fd) ) {
		throw LcyAsioException("eventfd close");
	}
}

static void write_eventfd(int fd)
{
	uint64_t u = 1;
	if ( ::write(fd, &u, sizeof(u)) != sizeof(u) ) {
		if ( errno != EAGAIN ) {
			throw LcyAsioException("eventfd write");
		}
	}
}

static void clear_eventfd(int fd)
{
	uint64_t u = 0;
	if ( ::read(fd, &u, sizeof(u)) != sizeof(u) ) {
		if ( errno != EAGAIN ) {
			throw LcyAsioException("eventfd read");
		}
	}
}

///////////////////////////////////////////////

BridgeService::BridgeService(ReactorService& reactor) :
	channel_(create_eventfd(), reactor)
{
	channel_.registerReadCallback(std::bind(		// FIXME : Check return value
		std::bind(&BridgeService::execute, this)));
}

BridgeService::~BridgeService()
{
	channel_.cancelReading();			// FIXME : Check return value
	destroy_eventfd(channel_.fd());
}

void BridgeService::pushNoLock(task_callback_type task_cb)
{
	task_list_.emplace_back(std::move(task_cb));
	write_eventfd(channel_.fd());
}

void BridgeService::push(task_callback_type task_cb)
{
	std::lock_guard<std::mutex> locker(mutex_);
	pushNoLock(std::move(task_cb));
}

ReactorService& BridgeService::reactor()
{
	return channel_.reactor();
}

void BridgeService::execute()
{
	std::lock_guard<std::mutex> locker(mutex_);
	
	clear_eventfd(channel_.fd());

	try {
		for ( auto& task_cb : task_list_ ) {
			task_cb();
		}
		task_list_.clear();
	} catch(...) {	// Nothing to do
		throw;
	}
}

}	// namespace details
}	// namespace asio
}	// namespace lcy
