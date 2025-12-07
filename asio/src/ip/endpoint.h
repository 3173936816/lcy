#ifndef __LCY_ASIO_IP_ENDPOINT_H__
#define __LCY_ASIO_IP_ENDPOINT_H__

#include <string>

#include "asio/src/ip/details/endpoint_data.h"

namespace lcy {
namespace asio {
namespace ip {

class EndPoint {
public:
	typedef uint16_t port_type;

	EndPoint();
	EndPoint(const void* native);
	EndPoint(const std::string& ip, port_type port);
	~EndPoint();

	bool operator==(const EndPoint& lhs) const;
	bool operator!=(const EndPoint& lhs) const;

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
