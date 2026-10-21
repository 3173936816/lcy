#ifndef __LCY_ASIO_IO_CONTEXT_HPP__
#define __LCY_ASIO_IO_CONTEXT_HPP__

#include <thread>
#include <cstdint>
#include <functional>

#include "lcy/noncopyable.hpp"
#include "lcy/asio/detail/scheduler.hpp"
#include "lcy/asio/detail/service_repository.hpp"

namespace lcy
{
namespace asio
{

class io_context :
	private noncopyable
{
public:
	typedef std::function<void()> task_func_type;

private:
	class task_operation
	{
	public:
		task_operation(io_context& ioc, task_func_type func) :
			self_(&ioc), func_(std::move(func)) {}

		void operator()(lcy::system::error_code, int) const
		{
			func_();
		}
		
	private:
		io_context* self_;
		task_func_type func_;
	};

public:
	template <typename Service>
	friend Service& use_service(io_context& ioc)
	{
		return ioc.repository_.use_service<Service>(ioc.sch_.get_reactor());
	}

	friend void post(io_context& ioc, task_func_type func)
	{
		task_operation task_op(ioc, std::move(func));
		ioc.sch_.post(detail::operation(std::move(task_op)));
	}

public:
	io_context(std::size_t concurrency_hint = std::thread::hardware_concurrency()) :
		sch_(concurrency_hint), repository_(concurrency_hint) {}

	~io_context()
	{
		repository_.cleanup_services();
		sch_.cleanup();
	}

	std::size_t run()
	{
		return sch_.run();		
	}

	void stop()
	{
		sch_.stop();
	}

private:
	typedef detail::scheduler scheduler;
	typedef detail::service_repository service_repository;

	scheduler sch_;
	service_repository repository_;
};

template <typename Service>
Service& use_service(io_context& ioc);

void post(io_context& ioc, io_context::task_func_type func);

}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_IO_CONTEXT_HPP__
