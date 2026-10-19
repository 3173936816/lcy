#ifndef __LCY_ASIO_DETAIL_CONDITIONALLY_MUTEX_HPP__
#define __LCY_ASIO_DETAIL_CONDITIONALLY_MUTEX_HPP__

#include <mutex>	// C++11 start

#include "lcy/noncopyable.hpp"

namespace lcy
{
namespace asio
{
namespace detail
{

class conditionally_mutex :
	private noncopyable
{
public:
	explicit conditionally_mutex(bool enable_lock = true)
		: enable_lock_(enable_lock) {}

	void lock()
	{
		if ( enable_lock_ ) {
			mtx_.lock();
		}
	}

	void unlock()
	{
		if ( enable_lock_ ) {
			mtx_.unlock();
		}
	}

	bool enable_lock() const
	{
		return enable_lock_;
	}

	std::mutex& get_mutex()
	{
		return mtx_;
	}

private:
	bool enable_lock_;
	std::mutex mtx_;
};

}	// namespace detail
}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_DETAIL_CONDITIONALLY_MUTEX_HPP__
