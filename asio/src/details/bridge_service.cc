#include "lcy/asio/src/details/bridge_service.hpp"
#include "lcy/asio/src/details/reactor_service.h"

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
	if ( fd != -1 ) {
		::close(fd);
	}
}

static void write_eventfd(int fd)
{
	uint64_t u = 1;
	::write(fd, &u, sizeof(u));
}

static void clear_eventfd(int fd)
{
	uint64_t u = 0;
	::read(fd, &u, sizeof(u));
}

///////////////////////////////////////////////

BridgeService::BridgeService(ReactorService& reactor) :
	event_fd_(create_eventfd()),
	reactor_(reactor)
{
	reactor_.registerReadOperation(event_fd_, std::bind(
		&BridgeService::execute, this, std::placeholders::_1));
}

BridgeService::~BridgeService()
{
	reactor_.removeAllOperations(event_fd_);
	destroy_eventfd(event_fd_);
}

void BridgeService::pushNoLock(task_op_type task_op)
{
	task_list_.emplace_back(std::move(task_op));
	write_eventfd(event_fd_);
}

void BridgeService::push(task_op_type task_op)
{
	std::lock_guard<std::mutex> locker(mutex_);
	pushNoLock(std::move(task_op));
}

ReactorService& BridgeService::reactor()
{
	return reactor_;
}

void BridgeService::execute(errcode_type ec)
{
	if ( !ec ) {
		task_list_type tmp_task_list;

		{
			std::lock_guard<std::mutex> locker(mutex_);
			clear_eventfd(event_fd_);
			tmp_task_list = std::move(task_list_);
			task_list_ = {};	// Reinitialize
		}

		for ( auto& task_op : tmp_task_list ) {
			task_op();
		}

	} else {
		/*
 		*notify:
		*	This function never fails
 		*/
	}
}

}	// namespace details
}	// namespace asio
}	// namespace lcy
