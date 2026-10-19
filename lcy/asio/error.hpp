#ifndef __LCY_ASIO_ERROR_HPP__
#define __LCY_ASIO_ERROR_HPP__

namespace lcy
{
namespace asio
{

#define LCY_ASIO_ERROR_NUMBER_MAP(XX)				\
	XX(success, 		0, Success)					\
	XX(operation_abort, 1, Operation abort)			\
	XX(already_open, 	2, Descriptor already open)	\
	XX(eof, 			3, Peer socket close)		\

enum error
{
#define XX(xx_errnum, xx_custom_num, xx_errmsg)	\
		xx_errnum = xx_custom_num,
	LCY_ASIO_ERROR_NUMBER_MAP(XX)
#undef XX
};

}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_ERROR_HPP__
