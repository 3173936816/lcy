#include "asio/src/ip/details/acceptor.h"
#include "asio/src/ip/tcp.h"
#include "asio/src/exception.h"

namespace lcy {
namespace asio {
namespace ip {
namespace details {

Acceptor::Acceptor(IOContext& ioc, const EndPoint& endpoint) :
	acceptor_(ioc)
{
	int errcode = 0;
	if ( endpoint.isV4() )
		errcode = acceptor_.open(TCP::v4());
	else if ( endpoint.isV6() )
		errcode == acceptor_.open(TCP::v6());
	else
		throw LcyAsioException("endpoint error");

	if ( errcode || 
		acceptor_.setReuseAddr() ||
		acceptor_.bind(endpoint) ||
		acceptor_.listen(4096) ||
		acceptor_.setDelay() ||
		acceptor_.setKeepAlive() ) {
		
		throw LcyAsioException("tcp_acceptor init");
	}
}

Acceptor::~Acceptor()
{
	cancel();
}

void Acceptor::async_accept(TCPSocket& socket, accept_op_type accept_op)
{
	acceptor_.async_accept(socket, std::move(accept_op));
}

void Acceptor::cancel()
{
	acceptor_.cancel();
}

}	// namespace details
}	// namespace ip
}	// namespace asio
}	// namespace lcy
