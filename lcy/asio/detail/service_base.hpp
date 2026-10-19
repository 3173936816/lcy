#ifndef __LCY_ASIO_DETAIL_SERVICE_BASE_HPP__
#define __LCY_ASIO_DETAIL_SERVICE_BASE_HPP__

#include "lcy/noncopyable.hpp"

namespace lcy
{
namespace asio
{
namespace detail
{

class service_base :
	private noncopyable
{
public:
	class service_id
	{
	public:
		service_id(void* id) :
			id_(id) {}
		
		operator void*() const
		{
			return id_;
		}

	private:
		void* id_;
	};

public:
	virtual ~service_base() {}
	
	virtual void cleanup() = 0;
	virtual service_id get_id() const = 0;
};

}	// namespace detail
}	// namespace asio
}	// anmespace lcy

#endif	// __LCY_ASIO_DETAIL_SERVICE_BASE_HPP__
