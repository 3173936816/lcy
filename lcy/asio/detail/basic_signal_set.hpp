#ifndef __LCY_ASIO_DETAIL_BASIC_SIGNAL_SET_HPP__
#define __LCY_ASIO_DETAIL_BASIC_SIGNAL_SET_HPP__

#include <initializer_list>

#include "lcy/noncopyable.hpp"
#include "lcy/asio/detail/signal_set_service.hpp"

namespace lcy
{
namespace asio
{
namespace detail
{

template <typename Executor>
class basic_signal_set :
	private noncopyable
{
public:
	typedef signal_set_service::signal_func_type signal_func_type;

	basic_signal_set(Executor& executor, std::initializer_list<int> signums) :
		executor_(executor), service_(use_service<signal_set_service>(executor))
	{
		service_.construct(impl_, signums);
	}

	~basic_signal_set()
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

	Executor& get_executor()
	{
		return executor_;
	}

private:
	typedef detail::signal_set_service::signal_set_impl signal_set_impl;
	
	Executor& executor_;
	signal_set_impl impl_;
	signal_set_service& service_;
};

}	// namespace detail
}   // namespace asio
}	// namespace lcy

#endif  // __LCY_ASIO_DETAIL_BASIC_SIGNAL_SET_HPP__