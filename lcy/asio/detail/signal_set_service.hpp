#ifndef __LCY_ASIO_DETAIL_SIGNAL_SET_SERVICE_HPP__
#define __LCY_ASIO_DETAIL_SIGNAL_SET_SERVICE_HPP__

#include <signal.h>
#include <functional>
#include <initializer_list>

#include "lcy/asio/detail/error.hpp"
#include "lcy/asio/detail/reactor.hpp"
#include "lcy/asio/detail/throw_asio_exception.hpp"
#include "lcy/asio/detail/service_registry_base.hpp"

namespace lcy
{
namespace asio
{
namespace detail
{

class signal_set_service :
	public service_registry_base<
		signal_set_service
	>
{
public:
	typedef std::function<
		void (
			lcy::system::error_code ec,
			int signum
		)
	> signal_func_type;

public:
	typedef reactor::internal_signal_set internal_signal_set;
	
	struct signal_set_impl
	{
		uint64_t sigmask_;
		internal_signal_set* data_;
	};

private:
	class signal_operation
	{
	public:
		signal_operation(signal_set_service* self, signal_func_type func) :
			self_(self), func_(std::move(func)) {}

		void operator()(lcy::system::error_code ec, int signum) const
		{
			func_(ec, signum);
		}

	private:
		signal_set_service* self_;
		signal_func_type func_;
	};

public:
	explicit signal_set_service(reactor& r) :
		reactor_(&r) {}

	void construct(signal_set_impl& impl, std::initializer_list<int> signums)
	{
		sigset_t mask;
		sigemptyset(&mask);
		uint64_t sigmask = 0;

		for ( auto signum : signums ) {
			if ( signum < SIGHUP || signum > SIGRTMAX ) {
				lcy::system::error_code ec(
					lcy::system::error::invalid_argument);
				throw_asio_exception(ec, "signal_set_service::construct");
			}

			sigaddset(&mask, signum);
			sigmask |= static_cast<uint64_t>(1) << (signum - 1);
		}

		::sigprocmask(SIG_BLOCK, &mask, 0);

		impl.sigmask_ = sigmask;
		impl.data_ = reactor_->alloc_internal_signal_set();
	}

	void destroy(signal_set_impl& impl)
	{
		reactor_->cancel_signal_set(impl.data_);
		reactor_->free_internal_signal_set(impl.data_);

		impl.sigmask_ = 0;
		impl.data_ = 0;
	}

	void async_wait(signal_set_impl& impl, signal_func_type func)
	{
		signal_operation signal_op(this, std::move(func));

		reactor_->capture_signal_set(impl.data_,
			impl.sigmask_, operation(std::move(signal_op)));
	}

	void cancel(signal_set_impl& impl)
	{
		reactor_->cancel_signal_set(impl.data_);
	}

	void cleanup() override
	{
		// nothing to do
	}

private:
	reactor* reactor_;
};

}	// namespace detail
}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_DETAIL_SIGNAL_SET_SERVICE_HPP__
