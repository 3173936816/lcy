#ifndef __LCY_ASIO_DETAIL_UNIX_ENDPOINT_HPP__
#define __LCY_ASIO_DETAIL_UNIX_ENDPOINT_HPP__

#include <string>
#include <cstring>
#include <sys/un.h>
#include <sys/socket.h>

#include "lcy/asio/detail/error.hpp"
#include "lcy/asio/detail/throw_asio_exception.hpp"

namespace lcy
{
namespace asio
{
namespace detail
{

class unix_endpoint
{
public:
	unix_endpoint()
	{
		::memset(&addr_, 0x00, sizeof(addr_));

		addr_.sun_family = AF_UNIX;
	}

	unix_endpoint(std::string path)
	{
		::memset(&addr_, 0x00, sizeof(addr_));

		if ( path.length() >= UNIX_PATH_MAX ) {
			lcy::system::error_code ec(
				lcy::system::error::filename_too_long);
			throw_asio_exception(ec, "unix_endpoint::unix_endpoint[path]");
		}

		addr_.sun_family = AF_UNIX;
		::memcpy(&addr_.sun_path[0], &path[0], path.length());
	}

	bool operator==(const unix_endpoint& rhs) const
	{
		return ::memcmp(&addr_, &rhs.addr_, sizeof(addr_)) == 0;
	}

	int family() const
	{
		return addr_.sun_family;
	}

	void* data()
	{
		return &addr_;
	}

	std::size_t size() const
	{
		return sizeof(addr_);
	}

	std::string to_string() const
	{
		return std::string(&addr_.sun_path[0]);
	}

private:
	sockaddr_un addr_;
};

}	// namespace detail
}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_DETAIL_UNIX_ENDPOINT_HPP__
