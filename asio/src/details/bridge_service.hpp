#ifndef __LCY_ASIO_DETAILS_BRIDGE_SERVICE_HPP__
#define __LCY_ASIO_DETAILS_BRIDGE_SERVICE_HPP__

#include "asio/src/details/service.hpp"
#include "asio/src/details/channel.h"
#include "asio/src/exception.h"

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
	typedef std::function<void ()> task_callback_type;

	BridgeService(ReactorService& reactor);
	~BridgeService();

	template <typename Iterator>
	void batchNoLock(Iterator beg, Iterator end);
	void pushNoLock(task_callback_type task_cb);

	template <typename Iterator>
	void batch(Iterator beg, Iterator end);
	void push(task_callback_type task_cb);

	ReactorService& reactor();

private:
	BridgeService(const BridgeService&);
	BridgeService& operator=(const BridgeService&);

	void execute();

private:
	typedef std::mutex mutex_type;
	typedef std::list<task_callback_type> task_list_type;

	mutex_type mutex_;
	Channel channel_;
	task_list_type task_list_;
};

LCY_ASIO_DETAILS_SERVICEID_REGISTER_EXTERN(BridgeService)

}	// namespace details
}	// namespace asio
}	// namespace lcy

#include "bridge_service.ipp"

#endif	// __LCY_ASIO_DETAILS_BRIDGE_SERVICE_HPP__
