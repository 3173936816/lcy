#ifndef __LCY_ASIO_DETAIL_THROW_ASIO_EXCEPTION_HPP__
#define __LCY_ASIO_DETAIL_THROW_ASIO_EXCEPTION_HPP__

#include <string>
#include <exception>

#include "lcy/asio/detail/error.hpp"

namespace lcy
{
namespace asio
{
namespace detail
{

class asio_exception :
	public std::exception
{
public:
	asio_exception(std::string errmsg) :
		errmsg_(std::move(errmsg)) {}

	const char* what() const noexcept override
	{
		return errmsg_.c_str();
	}

private:
	std::string errmsg_;
};

inline void throw_asio_exception(lcy::system::error_code ec, std::string usermsg)
{
	std::string msg = "asio:[" + usermsg + "]: " + ec.message();
	throw asio_exception(std::move(msg));
}

}	// namespace detail
}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_DETAIL_THROW_ASIO_EXCEPTION_HPP__
