#include "lcy/asio/src/ip/details/udp_socket.h"
#include "lcy/asio/src/ip/udp.h"
#include "lcy/asio/src/exception.h"

#include <unistd.h>
#include <fcntl.h>
#include <cstring>

namespace lcy {
namespace asio {
namespace ip {
namespace details {

static int create_udp_socket(int family)
{
	int sockfd = ::socket(family, SOCK_DGRAM, 0);
	if ( sockfd == -1 ) {
		return -1;
	}

	int flags = ::fcntl(sockfd, F_GETFL, 0);
	::fcntl(sockfd, F_SETFL, flags | O_NONBLOCK | O_CLOEXEC);
	
	return sockfd;
}

static int destroy_udp_socket(int fd)
{
	if ( fd != -1 && ::close(fd) ) {
		return errno;
	}
	return 0;
}

////////////////////////////////////////////////////

static void read_op_wrap(errcode_type ec,
						 int sockfd,
						 asio::details::ReactorService& reactor,
						 Endpoint& endpoint,
						 MutableBuffer mbuf,
						 UDPSocket::read_op_type read_op)
{
	if ( !ec ) {
		reactor.removeReadOperation(sockfd);

		socklen_t len = sizeof(struct sockaddr_in6);
		struct sockaddr* addr = (struct sockaddr*)endpoint.native();

		ssize_t nread = ::recvfrom(sockfd,
								   mbuf.data(), 
								   mbuf.length(),
								   MSG_NOSIGNAL,
								   addr, &len);

		if ( nread < 0 ) {
			read_op(errno, 0);	// error : errcode, 0
		} else {
			read_op(ec, nread); // success : 0, nread
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

static void write_op_wrap(errcode_type ec,
						  int sockfd,
						  asio::details::ReactorService& reactor,
						  Endpoint endpoint,
						  ConstBuffer cbuf,
						  size_t send_bytes,
						  UDPSocket::write_op_type write_op)
{
	if ( !ec ) {
		reactor.removeWriteOperation(sockfd);

		socklen_t len = endpoint.length();
		struct sockaddr* addr = (struct sockaddr*)endpoint.native();

		ssize_t nwrite = ::sendto(sockfd, 
								  cbuf.data(),
								  cbuf.length(),
								  MSG_NOSIGNAL,
								  addr, len);
		
		if ( nwrite == -1 ) {
			write_op(errno, send_bytes);
			return;
		}

		send_bytes += nwrite;

		if ( nwrite != cbuf.length() ) {
			reactor.registerWriteOperation(sockfd, std::bind(
				write_op_wrap, std::placeholders::_1, sockfd, std::ref(reactor), 
					endpoint, cbuf + nwrite, send_bytes, std::move(write_op)));
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

////////////////////////////////////////////////////

UDPSocket::UDPSocket(IOContext& ioc) :
	ioc_(ioc),
	sockfd_(-1),
	reactor_(use_service<asio::details::ReactorService>(ioc))
{
}

UDPSocket::~UDPSocket()
{
	shutdown();
}

void UDPSocket::async_read(Endpoint& endpoint, 
			    		   MutableBuffer mbuf, 
						   read_op_type read_op)
{
	reactor_.registerReadOperation(sockfd_, std::bind(
		read_op_wrap, std::placeholders::_1, sockfd_, 
			std::ref(reactor_), std::ref(endpoint), mbuf, std::move(read_op)));
}

void UDPSocket::async_write(const Endpoint& endpoint, 
		     				ConstBuffer cbuf,
				 			write_op_type write_op)
{
	reactor_.registerWriteOperation(sockfd_, std::bind(
		write_op_wrap, std::placeholders::_1, sockfd_, 
			std::ref(reactor_), endpoint, cbuf, 0, std::move(write_op)));
}

void UDPSocket::cancel()
{
	reactor_.cancelAllOperations(sockfd_);
}

errcode_type UDPSocket::open(const UDP& udp)
{
	if ( sockfd_ != -1 ) {
		shutdown();
	}

	sockfd_ = create_udp_socket(udp.family());
	if ( sockfd_ == -1 ) {
		return errno;
	}
	return 0;
}

errcode_type UDPSocket::bind(const Endpoint& endpoint)
{
	socklen_t len = endpoint.length();
	struct sockaddr* addr = (struct sockaddr*)endpoint.native();

	if ( ::bind(sockfd_, addr, len) ) {
		return errno;
	}
	return 0;
}

errcode_type UDPSocket::setReuseAddr()
{
	int flag = 1;
	if ( ::setsockopt(sockfd_, SOL_SOCKET, 
			SO_REUSEADDR, &flag, sizeof(int)) ) {
		return errno;
	}
	return 0;
}

errcode_type UDPSocket::shutdown()
{
	reactor_.cancelAllOperations(sockfd_);

	int ret = destroy_udp_socket(sockfd_);
	if ( ret ) {
		return errno;
	}

	sockfd_ = -1;
	return 0;
}

errcode_type UDPSocket::shutdownRead()
{
	reactor_.cancelReadOperation(sockfd_);

	if ( ::shutdown(sockfd_, SHUT_RD) ) {
		return errno;
	}
	return 0;
}

errcode_type UDPSocket::shutdownWrite()
{
	reactor_.cancelWriteOperation(sockfd_);

	if ( ::shutdown(sockfd_, SHUT_WR) ) {
		return errno;
	}
	return 0;
}

std::string UDPSocket::udpInfo()
{
    char buff[512] = { 0 };
    int sndbuf, rcvbuf;
    socklen_t len = sizeof(int);
    
    ::getsockopt(sockfd_, SOL_SOCKET, SO_SNDBUF, &sndbuf, &len);
    ::getsockopt(sockfd_, SOL_SOCKET, SO_RCVBUF, &rcvbuf, &len);
    
    ::snprintf(buff, sizeof(buff), "sndbuf=%d rcvbuf=%d", sndbuf, rcvbuf);
    return std::string(buff);
}

}	// namespace details
}	// namespace ip
}	// namespace asio
}	// namespace lcy
