#ifndef __LCY_ASIO_DETAILS_SERVICE_HPP__
#define __LCY_ASIO_DETAILS_SERVICE_HPP__

namespace lcy {
namespace asio {
namespace details {

class Service {
public:
	Service() {}
	virtual ~Service() {}
};


template <typename service_type>
class ServiceId {
public:
	static bool id;
};

template <typename service_type>
bool ServiceId<service_type>::id;


#define LCY_ASIO_DETAILS_SERVICEID_REGISTER(service)	\
	template class details::ServiceId<service>;

#define LCY_ASIO_DETAILS_SERVICEID_REGISTER_EXTERN(service)	\
	extern LCY_ASIO_DETAILS_SERVICEID_REGISTER(service)

}	// namespace details
}	// namespace asio
}	// namespace lcy


#endif	// __LCY_ASIO_DETAILS_SERVICE_HPP__
