#ifndef __LCY_ASIO_DETAIL_SOCKET_OPS_HPP__
#define __LCY_ASIO_DETAIL_SOCKET_OPS_HPP__

#include <poll.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include "lcy/asio/detail/error.hpp"

namespace lcy
{
namespace asio
{
namespace detail
{

namespace socket_ops
{

template <typename Protocol>
int open(Protocol proto, lcy::system::error_code& ec)
{
	int sockfd = ::socket(proto.family(),
		proto.type() | SOCK_NONBLOCK | SOCK_CLOEXEC, proto.protocol());
	if ( sockfd == -1 ) {
		ec = lcy::system::error_code(errno);
		return -1;
	}

	ec = lcy::system::error_code();

	return sockfd;
}

int close(int sockfd, lcy::system::error_code& ec)
{
	int res = ::close(sockfd);
	if ( res == -1 ) {
		ec = lcy::system::error_code(errno);
		return -1;
	}

	ec = lcy::system::error_code();

	return 0;
}

template <typename Endpoint>
int bind(int sockfd, Endpoint endpoint, lcy::system::error_code& ec)
{
	int res = ::bind(sockfd,
		static_cast<sockaddr*>(endpoint.data()), endpoint.size());
	if ( res == -1 ) {
		ec = lcy::system::error_code(errno);
		return -1;
	}

	ec = lcy::system::error_code();

	return 0;
}

int listen(int sockfd, int backlog, lcy::system::error_code& ec)
{
	int res = ::listen(sockfd, backlog);
	if ( res == -1 ) {
		ec = lcy::system::error_code(errno);
		return -1;
	}

	ec = lcy::system::error_code();

	return 0;
}

template <typename Endpoint>
int accept(int sockfd, Endpoint& endpoint, lcy::system::error_code& ec)
{
	socklen_t len = sizeof(endpoint);

	int new_sockfd = ::accept4(sockfd,
		static_cast<sockaddr*>(endpoint.data()), &len,
			SOCK_NONBLOCK | SOCK_CLOEXEC);
	if ( new_sockfd == -1 ) {
		ec = lcy::system::error_code(errno);
		return -1;
	}

	ec = lcy::system::error_code();

	return new_sockfd;
}

template <typename Endpoint>
int connect(int sockfd, Endpoint endpoint, lcy::system::error_code& ec)
{
	int res = ::connect(sockfd,
		static_cast<sockaddr*>(endpoint.data()), endpoint.size()); 
	if ( res == -1 ) {
		ec = lcy::system::error_code(errno);
		return -1;
	}

	ec = lcy::system::error_code();

	return 0;
}

bool is_stream_socket(int sockfd, lcy::system::error_code& ec)
{
	int type = 0;
	socklen_t len = sizeof(type);

	if ( ::getsockopt(sockfd, SOL_SOCKET, SO_TYPE, &type, &len) ) {
		ec = lcy::system::error_code(errno);
		return false;
	}

	ec = lcy::system::error_code();

	return type == SOCK_STREAM;
}

template <typename MutableBufferView>
ssize_t recv(int sockfd,
	MutableBufferView buffer_view, lcy::system::error_code& ec)
{
	ssize_t transferred_bytes = ::recv(sockfd,
		buffer_view.data(), buffer_view.length(), MSG_CMSG_CLOEXEC);
	if ( transferred_bytes == -1 ) {
		ec = lcy::system::error_code(errno);
	} else if ( transferred_bytes == 0 && is_stream_socket(sockfd, ec) ) {
		ec = make_asio_error_code(lcy::asio::error::eof);
	} else {
		ec = lcy::system::error_code();
	}

	return transferred_bytes;
}

template <typename ConstBufferView>
ssize_t send(int sockfd,
	ConstBufferView buffer_view, lcy::system::error_code& ec)
{
	ssize_t transferred_bytes = ::send(sockfd,
		buffer_view.data(), buffer_view.length(), MSG_NOSIGNAL);
	if ( transferred_bytes == -1 ) {
		ec = lcy::system::error_code(errno);
	} else {
		ec = lcy::system::error_code();
	}

	return transferred_bytes;
}

template <
	typename Endpoint,
	typename MutableBufferView
>
ssize_t recvfrom(int sockfd,
	Endpoint& endpoint, MutableBufferView buffer_view,
	lcy::system::error_code& ec)
{
	socklen_t len = sizeof(endpoint);

	ssize_t transferred_bytes = ::recvfrom(sockfd,
		buffer_view.data(), buffer_view.length(), MSG_CMSG_CLOEXEC,
			static_cast<sockaddr*>(endpoint.data()), &len);
	if ( transferred_bytes == -1 ) {
		ec = lcy::system::error_code(errno);
	} else {
		ec = lcy::system::error_code();
	}

	return transferred_bytes;
}

template <
	typename Endpoint,
	typename ConstBufferView
>
ssize_t sendto(int sockfd,
	Endpoint endpoint, ConstBufferView buffer_view,
	lcy::system::error_code& ec)
{
	ssize_t transferred_bytes = ::sendto(sockfd,
		buffer_view.data(), buffer_view.length(), MSG_NOSIGNAL,
			static_cast<sockaddr*>(endpoint.data()), endpoint.size());
	if ( transferred_bytes == -1 ) {
		ec = lcy::system::error_code(errno);
	} else {
		ec = lcy::system::error_code();
	}

	return transferred_bytes;
}

int setsockopt(int sockfd,
	int level, int optname, const void* optval,
	socklen_t optlen, lcy::system::error_code& ec)
{
	int res = ::setsockopt(sockfd, level, optname, optval, optlen);
	if ( res == -1 ) {
		ec = lcy::system::error_code(errno);
		return -1;
	}

	ec = lcy::system::error_code();

	return 0;
}

int getsockopt(int sockfd,
	int level, int optname, void* optval,
	socklen_t* optlen, lcy::system::error_code& ec)
{
	int res = ::getsockopt(sockfd, level, optname, optval, optlen);
	if ( res == -1 ) {
		ec = lcy::system::error_code(errno);
		return -1;
	}

	ec = lcy::system::error_code();

	return 0;
}

template <typename ...Args>
int ioctl(int sockfd,
	unsigned long request, lcy::system::error_code& ec, Args&&... args)
{
	int res = ::ioctl(sockfd, request, std::forward<Args>(args)...);
	if ( res == -1 ) {
		ec = lcy::system::error_code(errno);
		return -1;
	}

	ec = lcy::system::error_code();

	return 0;
}

template <typename Endpoint>
int getsockname(int sockfd,
	Endpoint& endpoint, lcy::system::error_code& ec)
{
	socklen_t len = sizeof(endpoint);

	int res = ::getsockname(sockfd,
		static_cast<sockaddr*>(endpoint.data()), &len);
	if ( res == -1 ) {
		ec = lcy::system::error_code(errno);
		return -1;
	}

	ec = lcy::system::error_code();

	return 0;
}

template <typename Endpoint>
int getpeername(int sockfd,
	Endpoint& endpoint, lcy::system::error_code& ec)
{
	socklen_t len = sizeof(endpoint);

	int res = ::getpeername(sockfd,
		static_cast<sockaddr*>(endpoint.data()), &len);
	if ( res == -1 ) {
		ec = lcy::system::error_code(errno);
		return -1;
	}

	ec = lcy::system::error_code();

	return 0;
}

int poll_read(int sockfd, lcy::system::error_code& ec)
{
	pollfd pfd;
	pfd.fd = sockfd;
	pfd.events = POLLIN;
	pfd.revents = 0;

	int res = ::poll(&pfd, 1, -1);
	if ( res == -1 ) {
		ec = lcy::system::error_code(errno);
		return -1;
	}

	ec = lcy::system::error_code();

	return 0;
}

int poll_write(int sockfd, lcy::system::error_code& ec)
{
	pollfd pfd;
	pfd.fd = sockfd;
	pfd.events = POLLOUT;
	pfd.revents = 0;

	int res = ::poll(&pfd, 1, -1);
	if ( res == -1 ) {
		ec = lcy::system::error_code(errno);
		return -1;
	}

	ec = lcy::system::error_code();

	return 0;
}

int poll_connect(int sockfd, lcy::system::error_code& ec)
{
	if ( poll_write(sockfd, ec) ) {
		return -1;
	}

	int error = 0;
	socklen_t len = sizeof(error);
	
	if ( getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len, ec) ) {
		return -1;
	}

	if ( error ) {
		ec = lcy::system::error_code(error);
		return -1;
	}

	ec = lcy::system::error_code();

	return 0;
}

template <
	typename Protocol,
	typename Endpoint,
	typename Container
>
int getaddrinfo(const std::string& host,
	const std::string& service, Protocol proto,
    Container& endpoints, lcy::system::error_code& ec)
{
	addrinfo hints;
    ::memset(&hints, 0x00, sizeof(hints));

    hints.ai_family = proto.family();
    hints.ai_socktype = proto.type();
    hints.ai_protocol = proto.protocol();
    hints.ai_flags = AI_PASSIVE;

	addrinfo* res = 0;
	int error = ::getaddrinfo(host.c_str(), service.c_str(), &hints, &res);
	if ( error ) {
		ec = make_asio_resolver_error_code(error);
		return error;
	}

	for ( addrinfo* ptr = res; ptr; ptr = ptr->ai_next ) {
        if ( ptr->ai_family == AF_INET ) {
			Endpoint endpoint;
            ::memcpy(endpoint.data(), ptr->ai_addr, sizeof(sockaddr_in));
            endpoints.push_back(endpoint);
        } 
		
		else if ( ptr->ai_family == AF_INET6 ) {
            Endpoint endpoint;
            ::memcpy(endpoint.data(), ptr->ai_addr, sizeof(sockaddr_in6));
            endpoints.push_back(endpoint);
        }
    }
    ::freeaddrinfo(res);

    ec = lcy::system::error_code();

    return 0;
}

}	// namespace socket_ops

}	// namespace detail
}	// namespace asio
}	// namsepace lcy

#endif	// __LCY_ASIO_DETAIL_SOCKET_OPS_HPP__
