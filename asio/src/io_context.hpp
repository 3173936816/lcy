#ifndef __LCY_ASIO_IO_CONTEXT_HPP__
#define __LCY_ASIO_IO_CONTEXT_HPP__

#include <mutex>
#include <thread>
#include <functional>
#include <unordered_map>

#include "lcy/asio/src/errinfo.h"
#include "lcy/asio/src/exception.h"
#include "lcy/asio/src/details/service.hpp"
#include "lcy/asio/src/details/reactor_service.h"
#include "lcy/asio/src/details/timer_service.h"
#include "lcy/asio/src/details/bridge_service.hpp"

namespace lcy {
namespace asio {

class IOContext {
public:
	template <typename service>
	friend service& use_service(IOContext& ioc);

	typedef std::function<void ()> task_op_type;
	friend void post(IOContext& ioc, task_op_type task_op);

	template <typename Iterator>
	friend void batch(IOContext& ioc, Iterator beg, Iterator end);

	IOContext();
	~IOContext();

	void quit();
	errcode_type loop_wait();

private:
	IOContext(const IOContext&);
	IOContext& operator=(const IOContext&);

private:
	typedef std::mutex mutex_type;
	typedef std::thread::id thread_id_type;
	typedef std::unordered_map<
				uintptr_t,
				details::Service*
			> id_service_umap_type;

	mutex_type mutex_;
	thread_id_type thread_id_;	
	id_service_umap_type id_service_umap_;
};

template <typename service>
service& use_service(IOContext& ioc);

typedef std::function<void ()> task_op_type;
void post(IOContext& ioc, task_op_type task_op);

template <typename Iterator>
void batch(IOContext& ioc, Iterator beg, Iterator end);

}	// namespace asio
}	// namespace lcy

#include "io_context.ipp"

#endif	// __LCY_ASIO_IO_CONTEXT_HPP__
