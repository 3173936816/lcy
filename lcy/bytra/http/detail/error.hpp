#ifndef __LCY_BYTRA_HTTP_DETAIL_ERROR_HPP__
#define __LCY_BYTRA_HTTP_DETAIL_ERROR_HPP__

#include "lcy/system/error.hpp"
#include "lcy/byrta/http/error.hpp"

namespace lcy
{
namespace bytra
{
namespace http
{
namespace detail
{

class http_category :
	public lcy::system::error_category
{
private:
	friend const lcy::system::error_category& fetch_http_category();
	
	http_category() {}

public:
	const char* name() const override
	{
		return "bytra.http";
	}

	const char* message(int errnum) const override
	{
		switch ( errnum ) {
		#define XX(xx_errnum, xx_custom_num, xx_errmsg)	\
				case xx_errnum : { return #xx_errmsg; break; }
			LCY_BYTRA_HTTP_ERROR_NUMBER_MAP(XX)
		#undef XX
			default : {
				return "Unknown error";
			}
		}	
	}
};

inline const lcy::system::error_category& fetch_http_category()
{
	static http_category m;
	return m;
}

inline lcy::system::error_code make_http_error_code(int errnum = error::success)
{
	return lcy::system::error_code(errnum, fetch_http_category());
}

}	// namespace detail
}   // namespace http
}	// namespace bytra
}	// namespace lcy

#endif	// __LCY_BYTRA_HTTP_DETAIL_ERROR_HPP__
