#include "asio/src/ip/details/tcp_socket.h"
#include "asio/src/exception.h"
#include "asio/src/errinfo.h"
#include "asio/src/ip/tcp.h"

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

////////////////////////////////////////////////////

static void read_cb_wrap(asio::details::Channel& channel,
						 MutableBuffer mbuf, 
						 TCPSocket::read_callback_type read_cb)
{
	channel.cancelReading();		// FIXME : check return value

	ssize_t nread = ::recv(channel.fd(), 
						   mbuf.data(), 
						   mbuf.length(), 
						   MSG_NOSIGNAL);

	if ( nread < 0 ) {
		read_cb(errno, nread);			// error : errcode, -1
	}
	else {
		read_cb(err::SUCCESS, nread);		// success : 0, nread    peer close : 0, 0
	}
}

/*
* example : 
*
* void read_cb(int errcode, ssize_t nbytes) {
*	if ( !errcode ) {
*		if ( nbytes == 0 ) {
*			socket.close();
*			return;
*		}
*	
*		process_data(buf);
*	} else {
*		printf(errinfo(errcode));
*	}
*
*/

static void write_cb_wrap(asio::details::Channel& channel, 
						  ConstBuffer cbuf,
						  size_t send_bytes,
						  TCPSocket::write_callback_type write_cb)
{
	channel.cancelWriting();		// FIXME : check return value

	ssize_t nwrite = ::send(channel.fd(), 
							cbuf.data(), 
							cbuf.length(), 
							MSG_NOSIGNAL);

	if ( nwrite == -1 ) {
		write_cb(errno, send_bytes);
		return;
	}

	send_bytes += nwrite;

	if ( nwrite != cbuf.length() ) {
		if( channel.registerWriteCallback(std::bind(
				write_cb_wrap, std::ref(channel), 
					cbuf + nwrite, send_bytes, write_cb)) ) {	// write_cb can not be moved
			write_cb(err::EREGEVENT, send_bytes);				// this needs to be used
		}
		return;
	}

	write_cb(err::SUCCESS, send_bytes);
}

/*
* example : 
*
* void write_cb(int errcode) {
*	if ( !errcode ) {
*		printf("all data send successful");
*	} else {
*		printf(errinfo(errcode));
*	}
* }
*
*/

static void accept_cb_wrap(asio::details::Channel& channel,
						   asio::details::Channel& accept_channel,
						   TCPSocket::accept_callback_type accept_cb)
{
	channel.cancelReading();	// FIXME : check return value

	int sockfd = ::accept(channel.fd(), nullptr, nullptr);
	if ( sockfd == -1 ) {
		accept_cb(errno);
		return;
	}

	accept_channel.setFd(sockfd);
	accept_cb(err::SUCCESS);
}

/*
* example : 
*
* void accept_cb(int errcode) {
*	if ( !errcode ) {
*		printf("accept ok");
*	} else {
*		printf(errinfo(errcode));
*	}
* }
*
*/

static void connect_cb_wrap(asio::details::Channel& channel,
							TCPSocket::connect_callback_type connect_cb)
{
	channel.cancelWriting();	// FIXME : check return value

	int errcode = 0;
	socklen_t len = sizeof(int);
	if ( ::getsockopt(channel.fd(), 
			SOL_SOCKET, SO_ERROR, &errcode, &len) ) {
		throw LcyAsioException("connect_cb_wrap");
	}
	
	connect_cb(errcode);
}

/*
* example : 
*
* void connect_cb(int errcode) {
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
	channel_(use_service<asio::details::ReactorService>(ioc))
{
}

TCPSocket::~TCPSocket()
{
	shutdown();
}

void TCPSocket::async_read(MutableBuffer mbuf, read_callback_type read_cb)
{
	if( channel_.registerReadCallback(std::bind(
			read_cb_wrap, std::ref(channel_), mbuf, read_cb)) ) {	// read_cb can not be moved
		read_cb(err::EREGEVENT, 0);                                 // this needs to be used
	}	
}

void TCPSocket::async_write(ConstBuffer cbuf, write_callback_type write_cb)
{
	if( channel_.registerWriteCallback(std::bind(
			write_cb_wrap, std::ref(channel_), cbuf, 0, write_cb)) ) {	// write_cb can not be move
		write_cb(err::EREGEVENT, 0);                                    // this needs to be used
	}	
}

void TCPSocket::async_accept(TCPSocket& tcp_socket, accept_callback_type accept_cb)
{
	if( channel_.registerReadCallback(std::bind(
			accept_cb_wrap, std::ref(channel_), 
				std::ref(tcp_socket.channel_), accept_cb)) ) {	// accept_cb can not be mo
		accept_cb(err::EREGEVENT);                              // this needs to be used
	}	
}

void TCPSocket::async_connect(const EndPoint& endpoint, connect_callback_type connect_cb)
{
	socklen_t len = endpoint.length(); 
	struct sockaddr* addr = (struct sockaddr*)endpoint.native();

	int ret = ::connect(channel_.fd(), addr, len);
	if ( ret == 0 ) {		// Succeed immediately
		connect_cb(err::SUCCESS);
		return;
	}
	
	if ( ret == -1 && errno != EINPROGRESS ) {		// An error occurred
		connect_cb(errno);
		return;
	}

	if( channel_.registerWriteCallback(std::bind(	// EINPROGRESS
			connect_cb_wrap, std::ref(channel_), connect_cb)) ) {	// connect_cb can not be move
		connect_cb(err::EREGEVENT);                                 // this needs to be used
	}	
}

void TCPSocket::cancel()
{
	channel_.cancelAll();		// FIXME : check return value
}

int TCPSocket::open(const TCP& tcp)
{
	if ( channel_.fd() != -1 )
		shutdown();

	int sockfd = create_tcp_socket(tcp.family());
	if ( sockfd == -1 )
		return errno;
	channel_.setFd(sockfd);

	return 0;
}

int TCPSocket::bind(const EndPoint& endpoint)
{
	socklen_t len = endpoint.length();
	struct sockaddr* addr = (struct sockaddr*)endpoint.native();

	if ( ::bind(channel_.fd(), addr, len) )
		return errno;

	return 0;
}

int TCPSocket::listen(int backlog)
{
	if ( ::listen(channel_.fd(), backlog) )
		return errno;
	return 0;
}

int TCPSocket::setDelay()
{
	int flag = 1;
	if ( ::setsockopt(channel_.fd(), IPPROTO_TCP, 
			TCP_NODELAY, &flag, sizeof(int)) )
		return errno;
	return 0;
}

int TCPSocket::setReuseAddr()
{
	int flag = 1;
	if ( ::setsockopt(channel_.fd(), SOL_SOCKET, 
			SO_REUSEADDR, &flag, sizeof(int)) )
		return errno;
	return 0;
}

int TCPSocket::setKeepAlive()
{
	int flag = 1;
	if ( ::setsockopt(channel_.fd(), SOL_SOCKET,
			SO_KEEPALIVE, &flag, sizeof(int)) )
		return errno;
	return 0;
}

int TCPSocket::shutdown()
{
	channel_.cancelAll();	// FIXME : check return value

	int ret = destroy_tcp_socket(channel_.fd());
	if ( ret )
		return errno;
	channel_.setFd(-1);
	return 0;
}

int TCPSocket::shutdownRead()
{
	channel_.cancelReading();	// FIXME : check return value

	if ( ::shutdown(channel_.fd(), SHUT_RD) )
		return errno;
	return 0;
}

int TCPSocket::shutdownWrite()
{
	channel_.cancelWriting();	// FIXME : check return value

	if ( ::shutdown(channel_.fd(), SHUT_WR) )
		return errno;
	return 0;
}

int TCPSocket::peerAddr(EndPoint& endpoint)
{
	socklen_t len = endpoint.bufferLen();
	struct sockaddr* addr = (struct sockaddr*)endpoint.native();

	if ( ::getpeername(channel_.fd(), addr, &len) )
		return errno;
	return 0;
}

int TCPSocket::localAddr(EndPoint& endpoint)
{
	socklen_t len = endpoint.bufferLen();
	struct sockaddr* addr = (struct sockaddr*)endpoint.native();

	if ( ::getsockname(channel_.fd(), addr, &len) )
		return errno;
	return 0;
}

std::string TCPSocket::tcpInfo()
{
	struct tcp_info tcpi;
	socklen_t len = sizeof(struct tcp_info);
	::memset(&tcpi, 0x00, len);
	
	::getsockopt(channel_.fd(), SOL_TCP, TCP_INFO, &tcpi, &len);
	
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

}	// namespace details
}	// namespace ip
}	// namespace asio
}	// namespace lcy
