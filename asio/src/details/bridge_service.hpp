#ifndef __LCY_ASIO_DETAILS_BRIDGE_SERVICE_HPP__
#define __LCY_ASIO_DETAILS_BRIDGE_SERVICE_HPP__

#include "lcy/asio/src/details/service.hpp"
#include "lcy/asio/src/exception.h"
#include "lcy/asio/src/errinfo.h"

#include <list>
#include <mutex>
#include <unistd.h>
#include <functional>

namespace lcy {
namespace asio {
namespace details {

class ReactorService;

class BridgeService :
	public Service
{
public:
	typedef std::function<void ()> task_op_type;

	BridgeService(ReactorService& reactor);
	~BridgeService();

	template <typename Iterator>
	void batchNoLock(Iterator beg, Iterator end);
	void pushNoLock(task_op_type task_op);

	template <typename Iterator>
	void batch(Iterator beg, Iterator end);
	void push(task_op_type task_op);

	ReactorService& reactor();

private:
	BridgeService(const BridgeService&);
	BridgeService& operator=(const BridgeService&);

	void execute(errcode_type ec);

private:
	typedef int eventfd_type;
	typedef std::mutex mutex_type;
	typedef std::list<task_op_type> task_list_type;

	mutex_type mutex_;
	eventfd_type event_fd_;
	ReactorService& reactor_;
	task_list_type task_list_;
};

LCY_ASIO_DETAILS_SERVICEID_REGISTER_EXTERN(BridgeService)

}	// namespace details
}	// namespace asio
}	// namespace lcy

#include "bridge_service.ipp"

#endif	// __LCY_ASIO_DETAILS_BRIDGE_SERVICE_HPP__
