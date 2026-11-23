#ifndef __LCY_BYTRA_HTTP_DETAIL_THROW_HTTP_EXCEPTION_HPP__
#define __LCY_BYTRA_HTTP_DETAIL_THROW_HTTP_EXCEPTION_HPP__

#include <string>
#include <exception>

#include "lcy/system/error.hpp"
#include "lcy/bytra/http/error.hpp"

namespace lcy
{
namespace bytra
{
namespace http
{
namespace detail
{

class http_exception :
	public std::exception
{
public:
	explicit http_exception(std::string errmsg) :
		errmsg_(std::move(errmsg)) {}

	const char* what() const noexcept override
	{
		return errmsg_.c_str();
	}

private:
	std::string errmsg_;
};

inline void throw_http_exception(lcy::system::error_code ec, std::string usermsg)
{
	std::string msg = "http:{" + usermsg + "}: " + ec.message();
	throw http_exception(std::move(msg));
}

}	// namespace detail
}   // namespace http
}	// namespace bytra
}	// namespace lcy

#endif	// __LCY_BYTRA_HTTP_DETAIL_THROW_HTTP_EXCEPTION_HPP__
