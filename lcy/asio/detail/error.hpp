#ifndef __LCY_ASIO_DETAIL_ERROR_HPP__
#define __LCY_ASIO_DETAIL_ERROR_HPP__

#include <netdb.h>

#include "lcy/asio/error.hpp"
#include "lcy/system/error.hpp"

namespace lcy
{
namespace asio
{
namespace detail
{

class asio_category :
	public lcy::system::error_category
{
private:
	friend const lcy::system::error_category& fetch_asio_category();
	
	asio_category() {}

public:
	const char* name() const override
	{
		return "asio";
	}

	const char* message(int errnum) const override
	{
		switch ( errnum ) {
		#define XX(xx_errnum, xx_custom_num, xx_errmsg)	\
				case xx_errnum : { return #xx_errmsg; break; }
			LCY_ASIO_ERROR_NUMBER_MAP(XX)
		#undef XX
			default : {
				return "Unknown error";
			}
		}	
	}
};

inline const lcy::system::error_category& fetch_asio_category()
{
	static asio_category m;
	return m;
}

inline lcy::system::error_code make_asio_error_code(int errnum = error::success)
{
	return lcy::system::error_code(errnum, fetch_asio_category());
}


class resolver_category
	: public lcy::system::error_category
{
private:
	friend const resolver_category& fetch_resolver_category();

	resolver_category() {}

public:
    const char* name() const override
	{ 
		return "asio resolver";
	}

    const char* message(int errnum) const override
	{
        return ::gai_strerror(errnum);
    }
};

inline const resolver_category& fetch_resolver_category()
{
    static resolver_category m;
    return m;
}

inline lcy::system::error_code make_asio_resolver_error_code(int errnum = error::success)
{
	return lcy::system::error_code(errnum, fetch_resolver_category());
}

}	// namespace detail
}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_ERROR_HPP__
