#ifndef __LCY_ASIO_DETAIL_SERVICE_REGISTRY_BASE_HPP__
#define __LCY_ASIO_DETAIL_SERVICE_REGISTRY_BASE_HPP__

#include "lcy/asio/detail/service_base.hpp"

namespace lcy
{
namespace asio
{
namespace detail
{

template <typename Service>
class service_registry_base :
	public service_base
{
public:
	service_id get_id() const override
	{
		return service_id(&id_);
	}

	static service_id id()
	{
		return service_id(&id_);
	}

private:
	static int id_;
};

template <typename Service>
int service_registry_base<Service>::id_ = 0;

}	// namespace detail
}	// namespace asio
}	// anmespace lcy

#endif	// __LCY_ASIO_DETAIL_SERVICE_REGISTRY_BASE_HPP__
