#ifndef __LCY_ASIO_DETAIL_SERVICE_REPOSITORY_HPP__
#define __LCY_ASIO_DETAIL_SERVICE_REPOSITORY_HPP__

#include <list>

#include "lcy/noncopyable.hpp"
#include "lcy/asio/detail/service_base.hpp"
#include "lcy/asio/detail/service_registry_base.hpp"
#include "lcy/asio/detail/conditionally_mutex.hpp"

namespace lcy
{
namespace asio
{
namespace detail
{

class service_repository :
	private noncopyable
{
public:
	service_repository(std::size_t concurrency_hint) :
		mutex_(concurrency_hint != 1) {}

	~service_repository()
	{
		for ( auto iter = services_.rbegin();
				iter != services_.rend(); ++iter ) {
			delete *iter;
		}
	}

	template <typename Service, typename ...Args>
	Service& use_service(Args&&... args)
	{
		std::lock_guard<conditionally_mutex> locker(mutex_);

		for ( auto service : services_ ) {
			if ( service->get_id() == service_registry_base<Service>::id() ) {
				return *static_cast<Service*>(service);
			}
		}

		Service* new_service = new Service(std::forward<Args>(args)...);
		services_.push_back(new_service);

		return *new_service;
	}

	void cleanup_services()
	{
		for ( auto iter = services_.rbegin();
				iter != services_.rend(); ++iter ) {
			(*iter)->cleanup();
		}
	}

private:
	conditionally_mutex mutex_;
	std::list<service_base*> services_;
};

}	// namespace detail
}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_DETAIL_SERVICE_REPOSITORY_HPP__
