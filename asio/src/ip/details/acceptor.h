#ifndef __LCY_ASIO_IP_DETAILS_ACCEPTOR_H__
#define __LCY_ASIO_IP_DETAILS_ACCEPTOR_H__

#include <functional>

#include "lcy/asio/src/errinfo.h"
#include "lcy/asio/src/io_context.hpp"
#include "lcy/asio/src/ip/endpoint.h"
#include "lcy/asio/src/ip/details/tcp_socket.h"

namespace lcy {
namespace asio {
namespace ip {
	class TCP;
namespace details {

class Acceptor {
public:
	typedef std::function<void (errcode_type)> accept_op_type;

	Acceptor(IOContext& ioc);
	Acceptor(IOContext& ioc, const Endpoint& endpoint);
	~Acceptor();

	errcode_type setup(const Endpoint& endpoint);
	void async_accept(TCPSocket& socket, accept_op_type accept_op);
	void cancel();

private:
	Acceptor(const Acceptor&);
	Acceptor& operator=(const Acceptor&);

private:
	TCPSocket acceptor_;
};

}	// namespace details
}	// namespace ip
}	// namespace asio
}	// namespace lcy

#endif // __LCY_ASIO_IP_DETAILS_ACCEPTOR_H__
