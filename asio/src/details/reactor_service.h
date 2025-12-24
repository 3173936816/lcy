#ifndef __LCY_ASIO_DETAILS_REACTOR_SERVICE_H__
#define __LCY_ASIO_DETAILS_REACTOR_SERVICE_H__

#include <vector>
#include <functional>
#include <sys/epoll.h>
#include <unordered_map>

#include "lcy/asio/src/errinfo.h"
#include "lcy/asio/src/details/service.hpp"

namespace lcy {
namespace asio {
namespace details {

class ReactorService :
	public Service 
{
public:
	typedef int file_descriptor_type;
	typedef std::function<void (errcode_type)> operation_type;

	ReactorService();
	~ReactorService();

	void quit();
	errcode_type loop_wait();

	void registerReadOperation(file_descriptor_type fd, operation_type op);
	void removeReadOperation(file_descriptor_type fd);
	void cancelReadOperation(file_descriptor_type fd);

	void registerWriteOperation(file_descriptor_type fd, operation_type op);
	void removeWriteOperation(file_descriptor_type fd);
	void cancelWriteOperation(file_descriptor_type fd);

	void removeAllOperations(file_descriptor_type fd);
	void cancelAllOperations(file_descriptor_type fd);

private:	
	ReactorService(const ReactorService&);
	ReactorService& operator=(const ReactorService&);

private:
	class OperationInfo;
	typedef int epollfd_type;
	typedef std::vector<struct epoll_event> event_array_type;
	typedef std::unordered_map<
				file_descriptor_type, 
				OperationInfo*
			> fd_opinfo_umap_type;
	
	bool quit_;
	epollfd_type epoll_fd_;
	event_array_type event_array_;
	fd_opinfo_umap_type fd_opinfo_umap_;
};

LCY_ASIO_DETAILS_SERVICEID_REGISTER_EXTERN(ReactorService)

}	// namespace details
}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_DETAILS_REACTOR_SERVICE_H__
