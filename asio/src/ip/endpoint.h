#ifndef __LCY_ASIO_IP_ENDPOINT_H__
#define __LCY_ASIO_IP_ENDPOINT_H__

#include <string>

#include "lcy/asio/src/ip/details/endpoint_data.h"

namespace lcy {
namespace asio {
namespace ip {

class Endpoint {
public:
	typedef uint16_t port_type;

	Endpoint();
	Endpoint(const void* native);
	Endpoint(const std::string& ip, port_type port);
	~Endpoint();

	bool operator==(const Endpoint& lhs) const;
	bool operator!=(const Endpoint& lhs) const;

	port_type port() const;
	std::string ip() const;

	bool isV4() const;
	bool isV6() const;

	socklen_t length() const;
	socklen_t bufferLen() const;

	const void* native() const;

private:
	details::data_type data_;
};

}	// namespace ip
}	// namespace asio
} 	// namespace lcy

#endif // __LCY_ASIO_IP_ENDPOINT_H__
