#ifndef __LCY_ASIO_IP_DETAILS_ACCEPTOR_H__
#define __LCY_ASIO_IP_DETAILS_ACCEPTOR_H__

#include <functional>

#include "asio/src/io_context.hpp"
#include "asio/src/ip/endpoint.h"
#include "asio/src/ip/details/tcp_socket.h"

namespace lcy {
namespace asio {
namespace ip {
namespace details {

class Acceptor {
public:
	typedef std::function<void (int)> accept_callback_type;

	Acceptor(IOContext& ioc, const EndPoint& endpoint);
	~Acceptor();

	void async_accept(TCPSocket& socket, accept_callback_type accept_cb);
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
