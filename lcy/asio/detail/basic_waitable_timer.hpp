#ifndef __LCY_ASIO_DETAIL_BASIC_WAITABLE_TIMER_HPP__
#define __LCY_ASIO_DETAIL_BASIC_WAITABLE_TIMER_HPP__

#include "lcy/noncopyable.hpp"
#include "lcy/asio/detail/throw_asio_exception.hpp"

namespace lcy
{
namespace asio
{
namespace detail
{

template <
	typename Executor,
	typename Service
>
class basic_waitable_timer :
	private noncopyable
{
public:
	typedef Executor executor_type;
	typedef Service service_type;

	typedef typename service_type::clock clock;
	typedef typename service_type::timer_func_type timer_func_type;

public:
	explicit basic_waitable_timer(executor_type& executor) :
		executor_(executor), service_(use_service<service_type>(executor))
	{
		service_.construct(impl_);
	}

	basic_waitable_timer(executor_type& executor, long long timeout) :
		executor_(executor), service_(use_service<service_type>(executor))
	{
		service_.construct(impl_);
		service_.expires_after(impl_, timeout);
	}

	~basic_waitable_timer()
	{
		service_.destroy(impl_);
	}

	void wait()
	{
		lcy::system::error_code ec;
		service_.wait(impl_, ec);
		if ( ec ) {
			detail::throw_asio_exception(ec, "basic_waitable_timer::wait");
		}
	}

	void wait(lcy::system::error_code& ec)
	{
		service_.wait(impl_, ec);
	}

	void async_wait(timer_func_type func)
	{
		service_.async_wait(impl_, std::move(func));
	}

	void cancel() 
	{
		service_.cancel(impl_);
	}

	void expires_after(long long timeout)
	{
		service_.expires_after(impl_, timeout);
	}
	
	void expires_at(long long expiry)
	{
		service_.expires_at(impl_, expiry);
	}

	long long expiry() const
	{
		return service_.expiry(impl_);
	}

	executor_type& get_executor()
	{
		return executor_;
	}

private:
	typedef typename service_type::timer_impl timer_impl;

	executor_type& executor_;
	timer_impl impl_;
	service_type& service_;
};

}	// namesapce detail
}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_DETAIL_BASIC_WAITABLE_TIMER_HPP__
