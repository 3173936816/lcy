#ifndef __LCY_ASIO_ERRINFO_H__
#define __LCY_ASIO_ERRINFO_H__

#include <string>

namespace lcy {
namespace asio {

#define LCY_ASIO_ERRINFO_MAP(XX)                        				\
    XX ( 0, 		SUCCESS, 	"Success" )								\
    XX ( -1001, 	ESYSTEM, 	"Internal system error" )				\
    XX ( -1002, 	EREGEVENT, 	"Event register error" )				\


enum err : 
	int 
{
#define XX(_code, _err, _info) \
	_err = _code,
	LCY_ASIO_ERRINFO_MAP(XX)
#undef XX
};

typedef int errcode_type;
std::string errinfo(errcode_type errcode);

}	// namespace asio
}	// namespace lcy

#endif // __LCY_ASIO_ERRINFO_H__
