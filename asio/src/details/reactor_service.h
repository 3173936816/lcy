#ifndef __LCY_ASIO_DETAILS_REACTOR_SERVICE_H__
#define __LCY_ASIO_DETAILS_REACTOR_SERVICE_H__

#include "asio/src/details/service.hpp"

#include <vector>
#include <sys/epoll.h>

namespace lcy {
namespace asio {
namespace details {

class Channel;

class ReactorService :
	public Service 
{
public:
	ReactorService();
	~ReactorService();

	void quit();
	void loop_wait();

	int registerChannel(Channel& channel);
	int removeChannel(Channel& channel);
	int modifyChannel(Channel& channel);

private:	
	ReactorService(const ReactorService&);
	ReactorService& operator=(const ReactorService&);

private:
	typedef int epollfd_type;
	typedef std::vector<struct epoll_event> event_array_type;
	
	bool quit_;
	epollfd_type epoll_fd_;
	event_array_type event_array_;
};

LCY_ASIO_DETAILS_SERVICEID_REGISTER_EXTERN(ReactorService)

}	// namespace details
}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_DETAILS_REACTOR_SERVICE_H__
