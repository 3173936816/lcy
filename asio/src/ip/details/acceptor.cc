#include "lcy/asio/src/ip/details/acceptor.h"
#include "lcy/asio/src/ip/tcp.h"
#include "lcy/asio/src/exception.h"

namespace lcy {
namespace asio {
namespace ip {
namespace details {

Acceptor::Acceptor(IOContext& ioc) :
	acceptor_(ioc)
{
}

Acceptor::Acceptor(IOContext& ioc, const Endpoint& endpoint) :
	acceptor_(ioc)
{
	if ( setup(endpoint) ) {
		throw LcyAsioException("acceptor init");
	}
}

Acceptor::~Acceptor()
{
	cancel();
}

errcode_type Acceptor::setup(const Endpoint& endpoint)
{	
	errcode_type ec = err::SUCCESS;

	if ( endpoint.isV4() ) {
		ec = acceptor_.open(TCP::v4());
	} else if ( endpoint.isV6() ) {
		ec = acceptor_.open(TCP::v6());
	} else {
		return EINVAL;
	}

	 if ( (ec) ||
		  (ec = acceptor_.setReuseAddr()) ||
		  (ec = acceptor_.bind(endpoint)) ||
		  (ec = acceptor_.listen(1024)) ||
		  (ec = acceptor_.setDelay()) ||
		  (ec = acceptor_.setKeepAlive())
	 );

	return ec;
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
