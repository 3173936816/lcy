#ifndef __LCY_ASIO_IP_UDP_H__
#define __LCY_ASIO_IP_UDP_H__

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>

#include "lcy/asio/src/ip/details/udp_socket.h"

namespace lcy {
namespace asio {
namespace ip {
namespace details {
	class UDPSocket;
}	// namespace details

class UDP {
public:
	typedef details::UDPSocket Socket;

public:
	static UDP v4()
	{
		return UDP(AF_INET);
	}

	static UDP v6()
	{
		return UDP(AF_INET6);
	}

	typedef int family_type;
	family_type family() const
	{
		return family_;
	}

private:
	UDP(family_type family) :
		family_(family)
	{
	}

private:
	family_type family_;
};

}	// namespace ip
}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_IP_UDP_H__
