#ifndef __LCY_ASIO_DETAIL_OPERATION_HPP__
#define __LCY_ASIO_DETAIL_OPERATION_HPP__

#include <functional>

#include "lcy/asio/detail/error.hpp"

namespace lcy
{
namespace asio
{
namespace detail
{

class operation
{
public:
	typedef std::function<
		void (
			lcy::system::error_code ec,
			int signum
		)
	> func_type;

	operation() : signum_(-1) {}

	operation(func_type func) :
		signum_(-1), func_(std::move(func)) {}

	void set_errorc(lcy::system::error_code ec)
	{
		ec_ = ec;
	}

	lcy::system::error_code get_errorc() const
	{
		return ec_;
	}

	void set_signum(int signum)
	{
		signum_ = signum;
	}

	int get_signum() const
	{
		return signum_;
	}

	void operator()() const
	{
		func_(ec_, signum_);
	}
	
private:
	int signum_;
	func_type func_;
	lcy::system::error_code ec_;
};

}	// namespace detail
}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_DETAIL_OPERATION_HPP__
