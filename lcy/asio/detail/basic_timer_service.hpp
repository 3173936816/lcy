#ifndef __LCY_ASIO_DETAIL_BASIC_TIMER_SERVICE_HPP__
#define __LCY_ASIO_DETAIL_BASIC_TIMER_SERVICE_HPP__

#include <functional>
#include <sys/select.h>

#include "lcy/asio/detail/reactor.hpp"
#include "lcy/asio/detail/operation.hpp"
#include "lcy/asio/detail/basic_timer_set.hpp"
#include "lcy/asio/detail/service_registry_base.hpp"

namespace lcy
{
namespace asio
{
namespace detail
{

template <typename Clock>
class basic_timer_service :
	public service_registry_base<
		basic_timer_service<Clock>
	>
{
public:
	typedef std::function<
		void (
			lcy::system::error_code ec
		)
	> timer_func_type;

public:
	typedef reactor::internal_timer internal_timer;

	struct timer_impl
	{
		long long expiry_;
		internal_timer* data_;
	};

private:
	class timer_operation
	{
	public:
		timer_operation(basic_timer_service* self, timer_func_type func) :
			self_(self), func_(std::move(func)) {}

		void operator()(lcy::system::error_code ec, int) const
		{
			func_(ec);
		}

	private:
		basic_timer_service* self_;
		timer_func_type func_;
	};

public:
	typedef Clock clock;

	basic_timer_service(reactor& r) :
		reactor_(&r)
	{
		reactor_->add_timer_set(&timer_set_);
	}

	~basic_timer_service()
	{
		reactor_->remove_timer_set(&timer_set_);
	}

	void construct(timer_impl& impl)
	{
		impl.expiry_ = 0;
		impl.data_ = reactor_->alloc_internal_timer();
	}

	void destroy(timer_impl& impl)
	{
		reactor_->cancel_timer(&timer_set_, impl.data_);
		reactor_->free_internal_timer(impl.data_);

		impl.expiry_ = 0;
		impl.data_ = 0;
	}

	void wait(timer_impl& impl, lcy::system::error_code& ec)
	{
		long long wait_ms = expiry(impl);

		if ( wait_ms > 0 ) {
			timeval tv;
			tv.tv_sec = wait_ms / 1000;
			tv.tv_usec = (wait_ms % 1000) * 1000;

			if ( ::select(0, 0, 0, 0, &tv) ) {
				ec = lcy::system::error_code(errno);
				return;
			}
		}

		ec = lcy::system::error_code();

		return;
	}

	void async_wait(timer_impl& impl, timer_func_type func)
	{
		timer_operation timer_op(this, std::move(func));

		reactor_->schedule_timer(&timer_set_,
			impl.data_, impl.expiry_, operation(std::move(timer_op)));
	}

	void cancel(timer_impl& impl) 
	{
		reactor_->cancel_timer(&timer_set_, impl.data_);
	}

	void expires_after(timer_impl& impl, long long timeout)
	{
		impl.expiry_ = Clock::now() + timeout;
	}
	
	void expires_at(timer_impl& impl, long long expiry)
	{
		impl.expiry_ = expiry;
	}

	long long expiry(const timer_impl& impl) const
	{
		return impl.expiry_ - Clock::now();
	}

private:
	reactor* reactor_;
	basic_timer_set<Clock> timer_set_;
};

}	// namespace detail
}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_DETAIL_BASIC_TIMER_SERVICE_HPP__
