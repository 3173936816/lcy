#ifndef __LCY_ASIO_IP_TCP_H__
#define __LCY_ASIO_IP_TCP_H__

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "lcy/asio/src/ip/details/tcp_socket.h"
#include "lcy/asio/src/ip/details/acceptor.h"

namespace lcy {
namespace asio {
namespace ip {

class TCP {
public:
	typedef details::TCPSocket Socket;
	typedef details::Acceptor Acceptor;

public:
	static TCP v4()
	{
		return TCP(AF_INET);
	}

	static TCP v6()
	{
		return TCP(AF_INET6);
	}

	typedef int family_type;
	family_type family() const
	{
		return family_;
	}

private:
	TCP(family_type family) :
		family_(family)
	{
	}

private:
	family_type family_;
};

}	// namespace ip
}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_IP_TCP_H__
