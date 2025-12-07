#include "asio/src/ip/endpoint.h"
#include "asio/src/exception.h"

#include <string.h>
#include <arpa/inet.h>

namespace lcy {
namespace asio {
namespace ip {

static bool ipv4_to_network(const std::string& ip, void* dest)
{
	return ::inet_pton(AF_INET, ip.c_str(), dest) == 1;
}

static bool ipv6_to_network(const std::string& ip, void* dest)
{
	return ::inet_pton(AF_INET6, ip.c_str(), dest) == 1;
}

static std::string network_to_ipv4(const void* src)
{
	std::string ipv4;
	ipv4.resize(INET_ADDRSTRLEN);

	if ( !::inet_ntop(AF_INET, src, &ipv4[0], ipv4.length()) ) {
		throw LcyAsioException("inet_ntop ipv4");
	}

	return ipv4;
}

static std::string network_to_ipv6(const void* src)
{
	std::string ipv6;
	ipv6.resize(INET6_ADDRSTRLEN);

	if ( !::inet_ntop(AF_INET6, src, &ipv6[0], ipv6.length()) ) {
		throw LcyAsioException("inet_ntop ipv6");
	}

	return ipv6;
}

//////////////////////////////////////////////

EndPoint::EndPoint()
{
	::memset(&data_, 0x00, sizeof(data_));
}

EndPoint::EndPoint(const void* native)
{
	::memset(&data_, 0x00, sizeof(data_));
	
	const sockaddr* addr = (const sockaddr*)native;
	if ( addr->sa_family == AF_INET ) {
		::memcpy(&data_.addrin_, native, sizeof(data_.addrin_));
	} else if (addr->sa_family == AF_INET6 ) {
		::memcpy(&data_.addrin6_, native, sizeof(data_.addrin6_));
	} else
		throw LcyAsioException("error addr");
}

EndPoint::EndPoint(const std::string& ip, port_type port)
{
	::memset(&data_, 0x00, sizeof(data_));
	
	if ( ipv4_to_network(ip, &data_.addrin_.sin_addr.s_addr) ) {
		data_.addrin_.sin_family = AF_INET;
		data_.addrin_.sin_port = htons(port);
	} else if ( ipv6_to_network(ip, &data_.addrin6_.sin6_addr.s6_addr) ) {
		data_.addrin6_.sin6_family = AF_INET6;
		data_.addrin6_.sin6_port = htons(port);
	} else
		throw LcyAsioException("error addr");
}

EndPoint::~EndPoint()
{
}

bool EndPoint::operator==(const EndPoint& lhs) const
{
	return ::memcmp(&data_, &lhs.data_, sizeof(data_)) == 0;
}

bool EndPoint::operator!=(const EndPoint& lhs) const
{
	return !(*this == lhs);
}

EndPoint::port_type EndPoint::port() const
{
	return ntohs(data_.addrin_.sin_port);
}

std::string EndPoint::ip() const
{
	const sockaddr* addr = (const sockaddr*)&data_;
	if ( addr->sa_family == AF_INET ) {
		return network_to_ipv4(&data_.addrin_.sin_addr.s_addr);
	} else if ( addr->sa_family == AF_INET6 ) {
		return network_to_ipv6(&data_.addrin6_.sin6_addr.s6_addr);
	} 
	
	return "";
}

bool EndPoint::isV4() const
{
	return ((struct sockaddr*)&data_)->sa_family == AF_INET;
}

bool EndPoint::isV6() const
{
	return ((struct sockaddr*)&data_)->sa_family == AF_INET6;
}

socklen_t EndPoint::length() const
{
	socklen_t len = 0;
	struct sockaddr* addr = (struct sockaddr*)&data_;
	
	if ( addr->sa_family == AF_INET )
		len = sizeof(struct sockaddr_in);
	else if ( addr->sa_family == AF_INET6 ) 
		len = sizeof(struct sockaddr_in6);

	return len;
}

socklen_t EndPoint::bufferLen() const
{
	return sizeof(data_);
}

const void* EndPoint::native() const
{
	return &data_;
}

}	// namespace ip
}	// namespace asio
}	// namespace lcy
