#ifndef __LCY_SSL_DETAIL_THROW_SSL_EXCEPTION_HPP__
#define __LCY_SSL_DETAIL_THROW_SSL_EXCEPTION_HPP__

#include <string>
#include <exception>

#include "lcy/ssl/detail/error.hpp"

namespace lcy
{
namespace ssl
{
namespace detail
{

class ssl_exception :
	public std::exception
{
public:
	explicit ssl_exception(std::string errmsg) :
		errmsg_(std::move(errmsg)) {}

	const char* what() const noexcept override
	{
		return errmsg_.c_str();
	}

private:
	std::string errmsg_;
};

inline void throw_ssl_exception(lcy::system::error_code ec, std::string usermsg)
{
	std::string msg = "ssl:{" + usermsg + "}: " + ec.message();
	throw ssl_exception(std::move(msg));
}

}	// namespace detail
}	// namespace ssl
}	// namespace lcy

#endif	// __LCY_SSL_DETAIL_THROW_SSL_EXCEPTION_HPP__
