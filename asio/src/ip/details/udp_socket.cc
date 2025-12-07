#include "asio/src/ip/details/udp_socket.h"
#include "asio/src/exception.h"
#include "asio/src/errinfo.h"

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

static void read_cb_wrap(asio::details::Channel& channel,
						 EndPoint& endpoint,
						 MutableBuffer mbuf,
						 UDPSocket::read_callback_type read_cb)
{
	channel.cancelReading();		// FIXME : check return value

	socklen_t len = sizeof(struct sockaddr_in6);
	struct sockaddr* addr = (struct sockaddr*)endpoint.native();

	ssize_t nread = ::recvfrom(channel.fd(), 
							   mbuf.data(), 
							   mbuf.length(),
							   MSG_NOSIGNAL,
							   addr, &len);

	if ( nread < 0 ) {
		read_cb(errno, nread);		// error : errcode, -1
	} else {
		read_cb(err::SUCCESS, nread);		// success : 0, nread    peer close : 0, 0
	}
}

static void write_cb_wrap(asio::details::Channel& channel,
						  EndPoint endpoint,
						  ConstBuffer cbuf,
						  size_t send_bytes,
						  UDPSocket::write_callback_type write_cb)
{
	channel.cancelWriting();		// FIXME : check return value

	socklen_t len = endpoint.length();
	struct sockaddr* addr = (struct sockaddr*)endpoint.native();

	ssize_t nwrite = ::sendto(channel.fd(), 
							  cbuf.data(),
							  cbuf.length(),
							  MSG_NOSIGNAL,
							  addr, len);
	
	if ( nwrite == -1 ) {
		write_cb(errno, send_bytes);
		return;
	}

	send_bytes += nwrite;

	if ( nwrite != cbuf.length() ) {
		if( channel.registerWriteCallback(std::bind(
				write_cb_wrap, std::ref(channel), endpoint,
					cbuf + nwrite, send_bytes, write_cb)) ) { // write_cb can not be moved
			write_cb(err::EREGEVENT, send_bytes);			  // this needs to be used
		}
		return;
	}

	write_cb(err::SUCCESS, send_bytes);
}

////////////////////////////////////////////////////

UDPSocket::UDPSocket(IOContext& ioc) :
	channel_(use_service<asio::details::ReactorService>(ioc))
{
}

UDPSocket::~UDPSocket()
{
	shutdown();
}

void UDPSocket::async_read(EndPoint& endpoint, 
			    		   MutableBuffer mbuf, 
						   read_callback_type read_cb)
{
	if ( channel_.registerReadCallback(std::bind(
			read_cb_wrap, std::ref(channel_), 
				std::ref(endpoint), mbuf, read_cb)) ) {		// read_cb can not be moved
		read_cb(err::EREGEVENT, 0);                         // this needs to be used
	}
}

void UDPSocket::async_write(const EndPoint& endpoint, 
		     				ConstBuffer cbuf, 
				 			write_callback_type write_cb)
{
	if ( channel_.registerWriteCallback(std::bind(
			write_cb_wrap, std::ref(channel_), 
				endpoint, cbuf, 0, write_cb)) ) {			// write_cb can not be moved
		write_cb(err::EREGEVENT, 0);                        // this needs to be used
	}
}

void UDPSocket::cancel()
{
	channel_.cancelAll();	// FIXME : check return value
}

int UDPSocket::open(UDP udp)
{
	if ( channel_.fd() != -1 )
		shutdown();

	int sockfd = create_udp_socket(udp.family());
	if ( sockfd == -1 )
		return errno;
	channel_.setFd(sockfd);

	return 0;
}

int UDPSocket::bind(const EndPoint& endpoint)
{
	socklen_t len = endpoint.length();
	struct sockaddr* addr = (struct sockaddr*)endpoint.native();

	if ( ::bind(channel_.fd(), addr, len) )
		return errno;

	return 0;
}

int UDPSocket::setReuseAddr()
{
	int flag = 1;
	if ( ::setsockopt(channel_.fd(), SOL_SOCKET, 
			SO_REUSEADDR, &flag, sizeof(int)) )
		return errno;
	return 0;
}

int UDPSocket::shutdown()
{
	channel_.cancelAll();	// FIXME : check return value

	int ret = destroy_udp_socket(channel_.fd());
	if ( ret )
		return errno;
	channel_.setFd(-1);
	return 0;
}

int UDPSocket::shutdownRead()
{
	channel_.cancelReading();	// FIXME : check return value

	if ( ::shutdown(channel_.fd(), SHUT_RD) )
		return errno;
	return 0;
}

int UDPSocket::shutdownWrite()
{
	channel_.cancelWriting();	// FIXME : check return value

	if ( ::shutdown(channel_.fd(), SHUT_WR) )
		return errno;
	return 0;
}

std::string UDPSocket::udpInfo()
{
    char buff[512] = { 0 };
    int sndbuf, rcvbuf;
    socklen_t len = sizeof(int);
    
    ::getsockopt(channel_.fd(), SOL_SOCKET, SO_SNDBUF, &sndbuf, &len);
    ::getsockopt(channel_.fd(), SOL_SOCKET, SO_RCVBUF, &rcvbuf, &len);
    
    ::snprintf(buff, sizeof(buff), "sndbuf=%d rcvbuf=%d", sndbuf, rcvbuf);
    return std::string(buff);
}

}	// namespace details
}	// namespace ip
}	// namespace asio
}	// namespace lcy
