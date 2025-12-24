#include "lcy/asio/src/details/reactor_service.h"
#include "lcy/asio/src/exception.h"

#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <iostream>

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
	if ( fd != -1 ) {
		::close(fd);
	}
}

static int epoll_register(int epollfd, int fd, int events, void* udata)
{
	struct epoll_event epevent;
	::memset(&epevent, 0x00, sizeof(epevent));
	
	epevent.events = events;
	if ( udata ) {
		epevent.data.ptr = udata;
	} else {
		epevent.data.fd = fd;
	}

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
		if ( errno != ENOENT || errno != EBADF ) {	// Errors can be ignored
			return errno;
		}
	}
	
	return 0;
}

static int epoll_modify(int epollfd, int fd, int events, void* udata)
{
	struct epoll_event epevent;
	::memset(&epevent, 0x00, sizeof(epevent));
	
	epevent.events = events;
	if ( udata ) {
		epevent.data.ptr = udata;
	} else {
		epevent.data.fd = fd;
	}

	if ( ::epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &epevent) ) {
		return errno;
	}
	
	return 0;
}

////////////////////////////////////////////////////////////

class ReactorService::OperationInfo {
public:
	typedef ReactorService::operation_type operation_type;

	OperationInfo();
	~OperationInfo();

	void setReadOperation(operation_type read_op);
	void removeReadOperation();
	void cancelReadOperation();
	void doReadOperation(errcode_type ec);
	
	void setWriteOperation(operation_type write_op);
	void removeWriteOperation();
	void cancelWriteOperation();
	void doWriteOperation(errcode_type ec);

	void removeAllOperations();
	void cancelAllOperations();

	bool hasReadOperation() const;
	bool hasWriteOperation() const;
	bool hasOperation() const;

private:
	operation_type read_op_;
	operation_type write_op_;
};

///////////////////////////////////////////////////////////

ReactorService::OperationInfo::OperationInfo()
{
}

ReactorService::OperationInfo::~OperationInfo()
{
}

void ReactorService::OperationInfo::setReadOperation(operation_type read_op)
{
	read_op_ = std::move(read_op);
}

void ReactorService::OperationInfo::removeReadOperation()
{
	read_op_ = {};
}

void ReactorService::OperationInfo::cancelReadOperation()
{
	if ( !hasReadOperation() ) return;
 /* 
 * notify :
 *	The reason for saving the callback first and then using tmp to execute it is to 
 *	prevent the user from re-registering it within the callback and being affected 
 *	or overwritten by subsequent operations.
 *
 *  **************************
 *	callback(err::EOPCANCELED);
 *	callback = {};
 *	**************************
 *
 *	This operation will overwrite the function re-registered by the user.
 */
	operation_type tmp_operation;
	std::swap(tmp_operation, read_op_);

	tmp_operation(err::EOPCANCELED);
}

void ReactorService::OperationInfo::doReadOperation(errcode_type ec)
{
	if ( hasReadOperation() ) {
		read_op_(ec);
	}
}

void ReactorService::OperationInfo::setWriteOperation(operation_type write_op)
{
	write_op_ = std::move(write_op);
}

void ReactorService::OperationInfo::removeWriteOperation()
{
	write_op_ = {};
}

void ReactorService::OperationInfo::cancelWriteOperation()
{
	if ( !hasWriteOperation() ) return;
 /* 
 * notify :
 *	The reason for saving the callback first and then using tmp to execute it is to 
 *	prevent the user from re-registering it within the callback and being affected 
 *	or overwritten by subsequent operations.
 *
 *  **************************
 *	callback(err::EOPCANCELED);
 *	callback = {};
 *	**************************
 *
 *	This operation will overwrite the function re-registered by the user.
 */
	operation_type tmp_operation;
	std::swap(tmp_operation, write_op_);

	tmp_operation(err::EOPCANCELED);
}

void ReactorService::OperationInfo::doWriteOperation(errcode_type ec)
{
	if ( hasWriteOperation() ) {
		write_op_(ec);
	}
}

void ReactorService::OperationInfo::removeAllOperations()
{
	removeReadOperation();
	removeWriteOperation();
}

void ReactorService::OperationInfo::cancelAllOperations()
{
	cancelReadOperation();
	cancelWriteOperation();
}

bool ReactorService::OperationInfo::hasReadOperation() const
{
	return read_op_ != nullptr;
}

bool ReactorService::OperationInfo::hasWriteOperation() const
{
	return write_op_ != nullptr;
}

bool ReactorService::OperationInfo::hasOperation() const
{
	return hasReadOperation() || hasWriteOperation();
}

////////////////////////////////////////////////////////////

ReactorService::ReactorService() :
	quit_(true),
	epoll_fd_(create_epollfd())
{
	event_array_.resize(128);
}

ReactorService::~ReactorService()
{
	for ( auto& kv : fd_opinfo_umap_ ) {
		OperationInfo* opinfo = kv.second;
		if ( opinfo->hasOperation() ) {
			epoll_remove(epoll_fd_, kv.first);
			opinfo->cancelAllOperations();
		}	// FIXME : check return value
		delete opinfo;
	}
	
	destroy_epollfd(epoll_fd_);
}

void ReactorService::quit()
{
	quit_ = true;
}

errcode_type ReactorService::loop_wait()
{
	quit_ = false;

	while ( !quit_ ) {
		int nevents = ::epoll_wait(epoll_fd_, 
			&event_array_[0], event_array_.size(), -1);
		if ( nevents < 0 ) {
			if ( errno == EINTR ) continue;
			else return errno;
		}
		
		for ( int i = 0; i < nevents; ++i ) {
			
			OperationInfo* opinfo = (OperationInfo*)event_array_[i].data.ptr;
			int events = event_array_[i].events;

			/*
 			*notify : 
 			* Multiple events may be triggered simultaneously,
 			* and the actions of earlier events may cancel or remove the actions of later events. 
 			* Therefore, it is necessary to check whether the subsequent operation exists before calling the operation function.
 			* We have made this check in OpInfo.
 			*/

			if ( events & (EPOLLERR | EPOLLHUP) ) {
				opinfo->doReadOperation(err::EFDHUP);
				opinfo->doWriteOperation(err::EFDHUP);
				continue;
			}

			if ( events & (EPOLLIN | EPOLLPRI) ) {
				opinfo->doReadOperation(err::SUCCESS);
			}
		
			if ( events & (EPOLLOUT) ) {
				opinfo->doWriteOperation(err::SUCCESS);
			}
		}

		if ( nevents >= event_array_.size() ) {
			event_array_.resize(nevents * 2);
		}
	}

	return 0;
}

void ReactorService::registerReadOperation(file_descriptor_type fd, operation_type op)
{
	OperationInfo* opinfo = nullptr;
	if ( fd_opinfo_umap_[fd] ) {
		opinfo = fd_opinfo_umap_[fd];
	} else {
		opinfo = new OperationInfo();
		fd_opinfo_umap_[fd] = opinfo;
	}

	if ( opinfo->hasReadOperation() ) {
		op(err::EOPEXISTS);
		return;
	}

	int events = EPOLLIN | EPOLLPRI;

	int errcode = err::SUCCESS;
	if ( opinfo->hasWriteOperation() ) {
		events |= EPOLLOUT;
		errcode = epoll_modify(epoll_fd_, fd, events, opinfo);
	} else {
		errcode = epoll_register(epoll_fd_, fd, events, opinfo);
	}

	if ( errcode ) {
		op(errcode);
		return;
	}
	
	opinfo->setReadOperation(std::move(op));
}

void ReactorService::removeReadOperation(file_descriptor_type fd)
{
	auto kv_iter = fd_opinfo_umap_.find(fd);
	if ( kv_iter == fd_opinfo_umap_.end() || 
		 !kv_iter->second->hasReadOperation() ) {
		return;
	}

	int events = 0;

	int errcode = err::SUCCESS;
	if ( kv_iter->second->hasWriteOperation() ) {
		events |= EPOLLOUT;
		errcode = epoll_modify(epoll_fd_, fd, events, kv_iter->second);
	} else {
		errcode = epoll_remove(epoll_fd_, fd);
	}		// FIXME : check errcode

	kv_iter->second->removeReadOperation();
}

void ReactorService::cancelReadOperation(file_descriptor_type fd)
{
	auto kv_iter = fd_opinfo_umap_.find(fd);
	if ( kv_iter == fd_opinfo_umap_.end() || 
		 !kv_iter->second->hasReadOperation() ) {
		return;
	}

	int events = 0;

	int errcode = err::SUCCESS;
	if ( kv_iter->second->hasWriteOperation() ) {
		events |= EPOLLOUT;
		errcode = epoll_modify(epoll_fd_, fd, events, kv_iter->second);
	} else {
		errcode = epoll_remove(epoll_fd_, fd);
	}		// FIXME : check errcode

	kv_iter->second->cancelReadOperation();
}

void ReactorService::registerWriteOperation(file_descriptor_type fd, operation_type op)
{
	OperationInfo* opinfo = nullptr;
	if ( fd_opinfo_umap_[fd] ) {
		opinfo = fd_opinfo_umap_[fd];
	} else {
		opinfo = new OperationInfo();
		fd_opinfo_umap_[fd] = opinfo;
	}

	if ( opinfo->hasWriteOperation() ) {
		op(err::EOPEXISTS);
		return;
	}
	
	int events = EPOLLOUT;

	int errcode = err::SUCCESS;
	if ( opinfo->hasReadOperation() ) {
		events |= EPOLLIN | EPOLLPRI;
		errcode = epoll_modify(epoll_fd_, fd, events, opinfo);
	} else {
		errcode = epoll_register(epoll_fd_, fd, events, opinfo);
	}

	if ( errcode ) {
		op(errcode);
		return;
	}
	
	opinfo->setWriteOperation(std::move(op));
}

void ReactorService::removeWriteOperation(file_descriptor_type fd)
{
	auto kv_iter = fd_opinfo_umap_.find(fd);
	if ( kv_iter == fd_opinfo_umap_.end() || 
		 !kv_iter->second->hasWriteOperation() ) {
		return;
	}

	int events = 0;

	int errcode = err::SUCCESS;
	if ( kv_iter->second->hasReadOperation() ) {
		events |= EPOLLIN | EPOLLPRI;
		errcode = epoll_modify(epoll_fd_, fd, events, kv_iter->second);
	} else {
		errcode = epoll_remove(epoll_fd_, fd);
	}		// FIXME : check errcode

	kv_iter->second->removeWriteOperation();
}

void ReactorService::cancelWriteOperation(file_descriptor_type fd)
{
	auto kv_iter = fd_opinfo_umap_.find(fd);
	if ( kv_iter == fd_opinfo_umap_.end() || 
		 !kv_iter->second->hasWriteOperation() ) {
		return;
	}

	int events = 0;

	int errcode = err::SUCCESS;
	if ( kv_iter->second->hasReadOperation() ) {
		events |= EPOLLIN | EPOLLPRI;
		errcode = epoll_modify(epoll_fd_, fd, events, kv_iter->second);
	} else {
		errcode = epoll_remove(epoll_fd_, fd);
	}		// FIXME : check errcode

	kv_iter->second->cancelWriteOperation();
}

void ReactorService::removeAllOperations(file_descriptor_type fd)
{
	auto kv_iter = fd_opinfo_umap_.find(fd);
	if ( kv_iter == fd_opinfo_umap_.end() ) {
		return;
	}

	int errcode = err::SUCCESS;
	if ( kv_iter->second->hasOperation() ) {
		errcode = epoll_remove(epoll_fd_, fd);
		kv_iter->second->removeAllOperations();
	}	// FIXME : check errcode
}

void ReactorService::cancelAllOperations(file_descriptor_type fd)
{
	auto kv_iter = fd_opinfo_umap_.find(fd);
	if ( kv_iter == fd_opinfo_umap_.end() ) {
		return;
	}

	int errcode = err::SUCCESS;
	if ( kv_iter->second->hasOperation() ) {
		errcode = epoll_remove(epoll_fd_, fd);
		kv_iter->second->cancelAllOperations();
	}	// FIXME : check errcode
}

}	// namespace details
}	// namespace asio
}	// namespace lcy
