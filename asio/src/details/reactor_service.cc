#include "asio/src/details/reactor_service.h"
#include "asio/src/details/channel.h"
#include "asio/src/exception.h"

#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

namespace lcy {
namespace asio {
namespace details {

LCY_ASIO_DETAILS_SERVICEID_REGISTER(ReactorService)

#define EPOLL_SIZE 2000

static int create_epollfd()
{	
	int epollfd = ::epoll_create(EPOLL_SIZE);
	if ( epollfd == -1 ) {
		throw LcyAsioException("epoll_create");
	} 
	
	int flags = ::fcntl(epollfd, F_GETFL, 0);
	::fcntl(epollfd, F_SETFL, EPOLL_CLOEXEC | flags);

	return epollfd;
}

static void destroy_epollfd(int fd)
{
	if ( fd != -1 && ::close(fd) ) {
		throw LcyAsioException("epollfd close");
	}
}

static int epoll_register(int epollfd, int fd, int events, void* udata)
{
	struct epoll_event epevent;
	::memset(&epevent, 0x00, sizeof(epevent));
	
	epevent.events = events;
	if ( udata )
		epevent.data.ptr = udata;
	else
		epevent.data.fd = fd;

	if ( ::epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &epevent) ) {
		return errno;
	}

	return 0;
}

static int epoll_remove(int epollfd, int fd)
{	
	struct epoll_event epevent;
	::memset(&epevent, 0x00, sizeof(epevent));
	
	if ( ::epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &epevent) ) {
		if ( errno != ENOENT )
			return errno;
	}
	
	return 0;
}

static int epoll_modify(int epollfd, int fd, int events, void* udata)
{
	struct epoll_event epevent;
	::memset(&epevent, 0x00, sizeof(epevent));
	
	epevent.events = events;
	if ( udata )
		epevent.data.ptr = udata;
	else
		epevent.data.fd = fd;

	if ( ::epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &epevent) ) {
		return errno;
	}
	
	return 0;
}

////////////////////////////////////////////////////////////

ReactorService::ReactorService() :
	quit_(true),
	epoll_fd_(-1)
{
	epoll_fd_ = create_epollfd();
	event_array_.resize(128);
}

ReactorService::~ReactorService()
{
	destroy_epollfd(epoll_fd_);
}

void ReactorService::quit()
{
	quit_ = true;
}

void ReactorService::loop_wait()
{
	quit_ = false;

	while ( !quit_ ) {
		int nevents = ::epoll_wait(epoll_fd_, 
			&event_array_[0], event_array_.size(), -1);
		if ( nevents < 0 ) {
			if ( errno == EINTR )
				continue;
			else
				throw LcyAsioException("epoll_wait");
		}
		
		for ( int i = 0; i < nevents; ++i ) {
			
			Channel* channel = (Channel*)event_array_[i].data.ptr;
			int events = event_array_[i].events;

			if ( events & (EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP) ) {
				(channel->readCallback())();
			}
		
			if ( events & (EPOLLOUT | EPOLLERR | EPOLLHUP) ) {
				(channel->writeCallback())();
			}
		}
		
		if ( nevents >= event_array_.size() )
			event_array_.resize(nevents * 2);
	}
}

int ReactorService::registerChannel(Channel& channel)
{
	int events = 0;
	if ( channel.readCallback() )
		events |= (EPOLLIN | EPOLLPRI);
	if ( channel.writeCallback() )
		events |= EPOLLOUT;

	return epoll_register(epoll_fd_, channel.fd(), events, &channel);
}

int ReactorService::removeChannel(Channel& channel)
{
	return epoll_remove(epoll_fd_, channel.fd());
}

int ReactorService::modifyChannel(Channel& channel)
{
	int events = 0;
	if ( channel.readCallback() )
		events |= (EPOLLIN | EPOLLPRI);
	if ( channel.writeCallback() )
		events |= EPOLLOUT;

	return epoll_modify(epoll_fd_, channel.fd(), events, &channel);
}

}	// namespace details
}	// namespace asio
}	// namespace lcy
