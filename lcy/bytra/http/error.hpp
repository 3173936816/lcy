#ifndef __LCY_BYTRA_HTTP_ERROR_HPP__
#define __LCY_BYTRA_HTTP_ERROR_HPP__

namespace lcy
{
namespace bytra
{
namespace http
{

#define LCY_BYTRA_HTTP_ERROR_NUMBER_MAP(XX)		    \
	XX(success,         0, Success)					\

enum error
{
#define XX(xx_errnum, xx_custom_num, xx_errmsg)	\
	    xx_errnum = xx_custom_num,
	LCY_BYTRA_HTTP_ERROR_NUMBER_MAP(XX)
#undef XX
};

}   // namespace http
}	// namespace asio
}	// namespace lcy

#endif	// __LCY_BYTRA_HTTP_ERROR_HPP__
