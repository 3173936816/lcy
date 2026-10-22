#ifndef __LCY_ASIO_DETAIL_BASIC_ENDPOINT_HPP__
#define __LCY_ASIO_DETAIL_BASIC_ENDPOINT_HPP__

#include <cstdint>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include "lcy/asio/detail/error.hpp"
#include "lcy/asio/detail/throw_asio_exception.hpp"

namespace lcy
{
namespace asio
{
namespace detail
{

template <typename InternetProtocol>
class basic_endpoint
{
public:
	typedef uint16_t port_type;
	typedef InternetProtocol protocol_type;

	basic_endpoint()
	{
		::memset(&addr_, 0x00, sizeof(addr_));

		addr_.v4_.sin_family = AF_INET;
		addr_.v4_.sin_port = 0;
		addr_.v4_.sin_addr.s_addr = INADDR_ANY;
	}

	explicit basic_endpoint(protocol_type proto)
	{
		::memset(&addr_, 0x00, sizeof(addr_));

		addr_.v4_.sin_family = proto.family();
		addr_.v4_.sin_port = 0;
		addr_.v4_.sin_addr.s_addr = INADDR_ANY;
	}

	basic_endpoint(protocol_type proto, port_type port_num)
	{
		::memset(&addr_, 0x00, sizeof(addr_));

		addr_.v4_.sin_family = proto.family();
		addr_.v4_.sin_port = ::htons(port_num);
		addr_.v4_.sin_addr.s_addr = INADDR_ANY;
	}

	basic_endpoint(std::string ip, port_type port_num)
	{
		::memset(&addr_, 0x00, sizeof(addr_));
		
		if ( inet_pton(AF_INET, &ip[0], &addr_.v4_.sin_addr.s_addr) == 1 ) {
			addr_.v4_.sin_family = AF_INET;
			addr_.v4_.sin_port = ::htons(port_num);

			return;
		}

		if ( inet_pton(AF_INET6, &ip[0], &addr_.v6_.sin6_addr.s6_addr) == 1 ) {
			addr_.v6_.sin6_family = AF_INET6;
			addr_.v6_.sin6_port = ::htons(port_num);

			return;
		}

		lcy::system::error_code ec(
			lcy::system::error::address_family_not_supported);
		throw_asio_exception(ec,
			"basic_endpoint::basic_endpoint[ip, port_num]");
	}

	bool operator==(const basic_endpoint& rhs) const
	{
		return ::memcmp(&addr_, &rhs.addr_, sizeof(addr_)) == 0;
	}

	bool is_v4() const
	{
		return addr_.v4_.sin_family == AF_INET;
	}

	int family() const
	{
		return addr_.v4_.sin_family;
	}

	void* data()
	{
		return &addr_;
	}

	std::size_t size() const
	{
		return is_v4() ? sizeof(sockaddr_in)
			: sizeof(sockaddr_in6);
	}

	std::string to_string() const
	{
		char ip[INET6_ADDRSTRLEN] = { 0 };

		if ( is_v4() ) {
			::inet_ntop(AF_INET, &addr_.v4_.sin_addr.s_addr, ip, sizeof(ip));
		} else {
			::inet_ntop(AF_INET6, &addr_.v6_.sin6_addr.s6_addr, ip, sizeof(ip));
		}

		std::string ip_port = std::string(ip) + ":" +
			std::to_string(::ntohs(addr_.v4_.sin_port));

		return ip_port;
	}

	protocol_type protocol() const
	{
		return is_v4() ? protocol_type::v4() :
			protocol_type::v6();
	}

private:
	union {
		sockaddr_in v4_;
		sockaddr_in6 v6_;
	} addr_;
};

}	// namespace detail
}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_DETAIL_BASIC_ENDPOINT_HPP__
