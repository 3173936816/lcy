#ifndef __LCY_ASIO_SIGNAL_SET_HPP__
#define __LCY_ASIO_SIGNAL_SET_HPP__

#include <initializer_list>

#include "lcy/noncopyable.hpp"
#include "lcy/asio/io_context.hpp"
#include "lcy/asio/detail/signal_set_service.hpp"

namespace lcy
{
namespace asio
{

class signal_set :
	private noncopyable
{
public:
	typedef detail::signal_set_service::signal_func_type signal_func_type;

	signal_set(io_context& ioc, std::initializer_list<int> signums) :
		ioc_(ioc), service_(use_service<detail::signal_set_service>(ioc))
	{
		service_.construct(impl_, signums);
	}

	~signal_set()
	{
		service_.destroy(impl_);
	}

	void async_wait(signal_func_type func)
	{
		service_.async_wait(impl_, std::move(func));
	}

	void cancel()
	{
		service_.cancel(impl_);
	}

	io_context& get_executor()
	{
		return ioc_;
	}

private:
	typedef detail::signal_set_service signal_set_service;
	typedef detail::signal_set_service::signal_set_impl signal_set_impl;
	
	io_context& ioc_;
	signal_set_impl impl_;
	signal_set_service& service_;
};

}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_SIGNAL_SET_HPP__
