#ifndef __LCY_SYSTEM_ERROR_HPP__
#define __LCY_SYSTEM_ERROR_HPP__

#include <errno.h>
#include <string.h>

#include "lcy/noncopyable.hpp"

namespace lcy
{
namespace system
{

#define LCY_SYSTEM_ERROR_NUMBER_MAP(XX)							\
	XX(success, 					 		0)					\
    XX(address_family_not_supported, 		EAFNOSUPPORT)		\
    XX(address_in_use, 				 		EADDRINUSE)			\
    XX(address_not_available, 		 		EADDRNOTAVAIL)		\
    XX(already_connected, 			 		EISCONN)			\
    XX(argument_list_too_long, 		 		E2BIG)				\
    XX(argument_out_of_domain, 		 		EDOM)				\
    XX(bad_address, 				 		EFAULT)				\
    XX(bad_file_descriptor, 		 		EBADF)				\
    XX(bad_message, 				 		EBADMSG)			\
    XX(broken_pipe, 				 		EPIPE)				\
    XX(connection_aborted, 			 		ECONNABORTED)		\
    XX(connection_already_in_progress,  	EALREADY)			\
    XX(connection_refused,					ECONNREFUSED)		\
    XX(connection_reset, 					ECONNRESET)			\
    XX(cross_device_link, 					EXDEV)				\
    XX(destination_address_required, 		EDESTADDRREQ)		\
    XX(device_or_resource_busy, 			EBUSY)				\
    XX(directory_not_empty, 				ENOTEMPTY)			\
    XX(executable_format_error, 			ENOEXEC)			\
    XX(file_exists, 						EEXIST)				\
    XX(file_too_large, 						EFBIG)				\
    XX(filename_too_long, 					ENAMETOOLONG)		\
    XX(function_not_supported, 				ENOSYS)				\
    XX(host_unreachable, 					EHOSTUNREACH)		\
    XX(identifier_removed, 					EIDRM)				\
    XX(illegal_byte_sequence, 				EILSEQ)				\
    XX(inappropriate_io_control_operation,  ENOTTY)				\
    XX(interrupted,							EINTR)				\
    XX(invalid_argument, 					EINVAL)				\
    XX(invalid_seek, 						ESPIPE)				\
    XX(io_error, 							EIO)				\
    XX(is_a_directory, 						EISDIR)				\
    XX(message_size, 						EMSGSIZE)			\
    XX(network_down, 						ENETDOWN)			\
    XX(network_reset, 						ENETRESET)			\
    XX(network_unreachable, 				ENETUNREACH)		\
    XX(no_buffer_space, 					ENOBUFS)			\
    XX(no_child_process, 					ECHILD)				\
    XX(no_link, 							ENOLINK)			\
    XX(no_lock_available, 					ENOLCK)				\
    XX(no_message_available, 				ENODATA)			\
    XX(no_message,							ENOMSG)				\
    XX(no_protocol_option, 					ENOPROTOOPT)		\
    XX(no_space_on_device, 					ENOSPC)				\
    XX(no_stream_resources, 				ENOSR)				\
    XX(no_such_device_or_address, 			ENXIO)				\
    XX(no_such_device, 						ENODEV)				\
    XX(no_such_file_or_directory, 			ENOENT)				\
    XX(no_such_process, 					ESRCH)				\
    XX(not_a_directory, 					ENOTDIR)			\
    XX(not_a_socket, 						ENOTSOCK)			\
    XX(not_a_stream, 						ENOSTR)				\
    XX(not_connected, 						ENOTCONN)			\
    XX(not_enough_memory, 					ENOMEM)				\
    XX(not_supported, 						ENOTSUP)			\
    XX(operation_canceled, 					ECANCELED)			\
    XX(operation_in_progress, 				EINPROGRESS)		\
    XX(operation_not_permitted, 			EPERM)				\
    XX(operation_not_supported, 			EOPNOTSUPP)			\
    XX(operation_would_block, 				EWOULDBLOCK)		\
    XX(owner_dead, 							EOWNERDEAD)			\
    XX(permission_denied, 					EACCES)				\
    XX(protocol_error, 						EPROTO)				\
    XX(protocol_not_supported, 				EPROTONOSUPPORT)	\
    XX(read_only_file_system, 				EROFS)				\
    XX(resource_deadlock_would_occur, 		EDEADLK)			\
    XX(resource_unavailable_try_again, 		EAGAIN)				\
    XX(result_out_of_range, 				ERANGE)				\
    XX(state_not_recoverable, 				ENOTRECOVERABLE)	\
    XX(stream_timeout, 						ETIME)				\
    XX(text_file_busy, 						ETXTBSY)			\
    XX(timed_out, 							ETIMEDOUT)			\
    XX(too_many_files_open_in_system, 		ENFILE)				\
    XX(too_many_files_open, 				EMFILE)				\
    XX(too_many_links, 						EMLINK)				\
    XX(too_many_symbolic_link_levels, 		ELOOP)				\
    XX(value_too_large, 					EOVERFLOW)			\
    XX(wrong_protocol_type, 				EPROTOTYPE)			\


enum error
{
#define XX(xx_errnum, xx_system_errnum)	\
		xx_errnum = xx_system_errnum,
	LCY_SYSTEM_ERROR_NUMBER_MAP(XX)
#undef XX
};

class error_category :
	private noncopyable
{
protected:
	error_category() {}
	virtual ~error_category() {}

public:
	virtual const char* name() const = 0;
	virtual const char* message(int errnum) const = 0;
};

class system_category :
	public error_category
{
private:
	friend const system_category& fetch_system_category();

	system_category() {}

public:
	const char* name() const override
	{
		return "system";
	}

	const char* message(int errnum) const override
	{
		return ::strerror(errnum);
	}
};

inline const system_category& fetch_system_category()
{
	static system_category m;
	return m;
}

class error_code
{
public:
	explicit error_code(int errnum = 0) :
		errnum_(errnum), category_(&fetch_system_category()) {}

	error_code(int errnum, const error_category& category) :
		errnum_(errnum), category_(&category) {}

	const char* message() const
	{
		return category_->message(errnum_);
	}

	const error_category& get_category() const
	{
		return *category_;
	}

	operator int() const
	{
		return errnum_;
	}

	bool operator==(const error_code& rhs) const
	{
		return errnum_ == rhs.errnum_
			&& category_ == rhs.category_;
	}

	bool operator!=(const error_code& rhs) const
	{
		return !(*this == rhs);
	}

private:
	int errnum_;
	const error_category* category_;
};

}	// namespace system
}	// namespace lcy

#endif	// __LCY_SYSTEM_ERROR_HPP__
