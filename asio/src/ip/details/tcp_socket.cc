#include "lcy/asio/src/ip/details/tcp_socket.h"
#include "lcy/asio/src/exception.h"
#include "lcy/asio/src/ip/tcp.h"

#include <unistd.h>
#include <fcntl.h>
#include <cstring>

namespace lcy {
namespace asio {
namespace ip {
namespace details {

static int create_tcp_socket(int family)
{
	int sockfd = ::socket(family, SOCK_STREAM, 0);
	if ( sockfd == -1 ) {
		return -1;
	}

	int flags = ::fcntl(sockfd, F_GETFL, 0);
	::fcntl(sockfd, F_SETFL, flags | O_NONBLOCK | O_CLOEXEC);
	
	return sockfd;
}

static int destroy_tcp_socket(int fd)
{
	if ( fd != -1 && ::close(fd) ) {
		return errno;
	}
	return 0;
}

//////////////////////////////////////////////////////////

static void read_op_wrap(errcode_type ec,
						 int sockfd,
						 asio::details::ReactorService& reactor,
						 MutableBuffer mbuf, 
						 TCPSocket::read_op_type read_op)
{
	if ( !ec ) {
		reactor.removeReadOperation(sockfd);

		ssize_t nread = ::recv(sockfd, 
							   mbuf.data(), 
							   mbuf.length(), 
							   MSG_NOSIGNAL);

		if ( nread < 0 ) {
			read_op(errno, 0);	// error : errcode, 0
		} else {
			read_op(ec, nread); // success : 0, nread    peer close : 0, 0
		}
	} else {
		 /*
 		 *notify:
 		 *	This error was notified by the reactor ( mybe EOPEXISTS, EOPCANCELED )
 		 *	We just need to notify the caller.
 		 */	

		read_op(ec, 0);
	}
}

/*
* example : 
*
* void read_op(int errcode, size_t nbytes) {
*	if ( !errcode ) {
*		if ( nbytes == 0 ) {
*			socket.shutdown();
*			return;
*		}
*	
*		process_data(buf);
*	} else {
*		printf(errinfo(errcode));
*	}
*
*/

static void write_op_wrap(errcode_type ec,
						  int sockfd,
						  asio::details::ReactorService& reactor,
						  ConstBuffer cbuf,
						  size_t send_bytes,
						  TCPSocket::write_op_type write_op)
{
	if ( !ec ) { 
		reactor.removeWriteOperation(sockfd);

		ssize_t nwrite = ::send(sockfd,
								cbuf.data(), 
								cbuf.length(), 
								MSG_NOSIGNAL);

		if ( nwrite < 0 ) {
			write_op(errno, send_bytes);
			return;
		}

		send_bytes += nwrite;

		if ( nwrite != cbuf.length() ) {
			reactor.registerWriteOperation(sockfd, std::bind(
				write_op_wrap, std::placeholders::_1, sockfd,
					std::ref(reactor), cbuf + nwrite, send_bytes, std::move(write_op)));
			return;
		}

		write_op(ec, send_bytes);
	} else {
	    /*
 		 *notify:
 		 *	This error was notified by the reactor ( mybe EOPEXISTS, EOPCANCELED )
 		 *	We just need to notify the caller.
 		 */	
	
		write_op(ec, send_bytes);
	}
}

/*
* example : 
*
* void write_op(int errcode, size_t nbytes) {
*	if ( !errcode ) {
*		printf("%ld bytes data send successful", nbytes);
*	} else {
*		printf(errinfo(errcode));
*	}
* }
*
*/

static void accept_op_wrap(errcode_type ec,
						   int sockfd,
						   int& accept_sockfd,
						   asio::details::ReactorService& reactor,
						   TCPSocket::accept_op_type accept_op)
{
	if ( !ec ) {
		reactor.removeReadOperation(sockfd);

		accept_sockfd = ::accept(sockfd, nullptr, nullptr);
		if ( accept_sockfd == -1 ) {
			accept_op(errno);
			return;
		}

		accept_op(ec);
	} else {
 		/*
 		 *notify:
 		 *	This error was notified by the reactor ( mybe EOPEXISTS, EOPCANCELED )
 		 *	We just need to notify the caller.
 		 */	

		accept_op(ec);
	}
}

/*
* example : 
*
* void accept_op(int errcode) {
*	if ( !errcode ) {
*		printf("accept ok");
*	} else {
*		printf(errinfo(errcode));
*	}
* }
*
*/

static void connect_op_wrap(errcode_type ec,
							int sockfd,
							asio::details::ReactorService& reactor,
							TCPSocket::connect_op_type connect_op)
{
	if ( !ec ) {
		reactor.removeWriteOperation(sockfd);

		int errcode = 0;
		socklen_t len = sizeof(int);
		::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &errcode, &len);
		
		connect_op(errcode);
	} else {
		/*
 		 *notify:
 		 *	This error was notified by the reactor ( mybe EOPEXISTS, EOPCANCELED )
 		 *	We just need to notify the caller.
 		 */	

		connect_op(ec);
	}
}

/*
* example : 
*
* void connect_op(int errcode) {
*	if ( !errcode ) {
*		printf("connect ok");
*	} else {
*		printf(errinfo(errcode));
*	}
* }
*
*/

////////////////////////////////////////////////////

TCPSocket::TCPSocket(IOContext& ioc) :
	ioc_(ioc),
	sockfd_(-1),
	reactor_(use_service<asio::details::ReactorService>(ioc))
{
}

TCPSocket::~TCPSocket()
{
	shutdown();
}

void TCPSocket::async_read(MutableBuffer mbuf, read_op_type read_op)
{
	reactor_.registerReadOperation(sockfd_, std::bind(
		read_op_wrap, std::placeholders::_1, sockfd_, 
			std::ref(reactor_), mbuf, std::move(read_op)));
}

void TCPSocket::async_write(ConstBuffer cbuf, write_op_type write_op)
{
	reactor_.registerWriteOperation(sockfd_, std::bind(
		write_op_wrap, std::placeholders::_1, sockfd_, 
			std::ref(reactor_), cbuf, 0, std::move(write_op)));
}

void TCPSocket::async_accept(TCPSocket& tcp_socket, accept_op_type accept_op)
{
	reactor_.registerReadOperation(sockfd_, std::bind(
		accept_op_wrap, std::placeholders::_1, sockfd_, 
			std::ref(tcp_socket.sockfd_), std::ref(reactor_), std::move(accept_op)));
}

void TCPSocket::async_connect(const Endpoint& endpoint, connect_op_type connect_op)
{
	socklen_t len = endpoint.length(); 
	struct sockaddr* addr = (struct sockaddr*)endpoint.native();

	int ret = ::connect(sockfd_, addr, len);
	if ( ret == 0 ) {		// Succeed immediately
		connect_op(err::SUCCESS);
		return;
	}
	
	if ( ret == -1 && errno != EINPROGRESS ) {		// An error occurred
		connect_op(errno);
		return;
	}

	reactor_.registerWriteOperation(sockfd_, std::bind(		// EINPROGRESS
		connect_op_wrap, std::placeholders::_1, sockfd_, 
			std::ref(reactor_), std::move(connect_op)));
}

void TCPSocket::cancel()
{
	reactor_.cancelAllOperations(sockfd_);
}

errcode_type TCPSocket::open(const TCP& tcp)
{
	if ( sockfd_ != -1 ) {
		shutdown();
	}

	sockfd_ = create_tcp_socket(tcp.family());
	if ( sockfd_ == -1 ) {
		return errno;
	}
	return 0;
}

errcode_type TCPSocket::bind(const Endpoint& endpoint)
{
	socklen_t len = endpoint.length();
	struct sockaddr* addr = (struct sockaddr*)endpoint.native();

	if ( ::bind(sockfd_, addr, len) ) {
		return errno;
	}
	return 0;
}

errcode_type TCPSocket::listen(int backlog)
{
	if ( ::listen(sockfd_, backlog) ) {
		return errno;
	}
	return 0;
}

errcode_type TCPSocket::setDelay()
{
	int flag = 1;
	if ( ::setsockopt(sockfd_, IPPROTO_TCP, 
			TCP_NODELAY, &flag, sizeof(int)) ) {
		return errno;
	}
	return 0;
}

errcode_type TCPSocket::setReuseAddr()
{
	int flag = 1;
	if ( ::setsockopt(sockfd_, SOL_SOCKET, 
			SO_REUSEADDR, &flag, sizeof(int)) ) {
		return errno;
	}
	return 0;
}

errcode_type TCPSocket::setKeepAlive()
{
	int flag = 1;
	if ( ::setsockopt(sockfd_, SOL_SOCKET,
			SO_KEEPALIVE, &flag, sizeof(int)) ) {
		return errno;
	}
	return 0;
}

errcode_type TCPSocket::shutdown()
{
	reactor_.cancelAllOperations(sockfd_);

	int ret = destroy_tcp_socket(sockfd_);
	if ( ret ) {
		return errno;
	}

	sockfd_ = -1;
	return 0;
}

errcode_type TCPSocket::shutdownRead()
{
	reactor_.cancelReadOperation(sockfd_);

	if ( ::shutdown(sockfd_, SHUT_RD) ) {
		return errno;
	}
	return 0;
}

errcode_type TCPSocket::shutdownWrite()
{
	reactor_.cancelWriteOperation(sockfd_);

	if ( ::shutdown(sockfd_, SHUT_WR) ) {
		return errno;
	}
	return 0;
}

errcode_type TCPSocket::peerAddr(Endpoint& endpoint)
{
	socklen_t len = endpoint.bufferLen();
	struct sockaddr* addr = (struct sockaddr*)endpoint.native();

	if ( ::getpeername(sockfd_, addr, &len) ) {
		return errno;
	}
	return 0;
}

errcode_type TCPSocket::localAddr(Endpoint& endpoint)
{
	socklen_t len = endpoint.bufferLen();
	struct sockaddr* addr = (struct sockaddr*)endpoint.native();

	if ( ::getsockname(sockfd_, addr, &len) ) {
		return errno;
	}
	return 0;
}

std::string TCPSocket::tcpInfo()
{
	struct tcp_info tcpi;
	socklen_t len = sizeof(struct tcp_info);
	::memset(&tcpi, 0x00, len);
	
	::getsockopt(sockfd_, SOL_TCP, TCP_INFO, &tcpi, &len);
	
	char buff[1024] = { 0 };
	::snprintf(buff, sizeof(buff), "unrecovered=%u "
               "rto=%u ato=%u snd_mss=%u rcv_mss=%u "
               "lost=%u retrans=%u rtt=%u rttvar=%u "
               "sshthresh=%u cwnd=%u total_retrans=%u",
               tcpi.tcpi_retransmits,
               tcpi.tcpi_rto,
               tcpi.tcpi_ato,
               tcpi.tcpi_snd_mss,
               tcpi.tcpi_rcv_mss,
               tcpi.tcpi_lost,
               tcpi.tcpi_retrans,
               tcpi.tcpi_rtt,
               tcpi.tcpi_rttvar,
               tcpi.tcpi_snd_ssthresh,
               tcpi.tcpi_snd_cwnd,
               tcpi.tcpi_total_retrans);
	return std::string(buff);
}

IOContext& TCPSocket::context()
{
	return ioc_;
}

}	// namespace details
}	// namespace ip
}	// namespace asio
}	// namespace lcy
