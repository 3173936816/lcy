#ifndef __LCY_ASIO_ERRINFO_H__
#define __LCY_ASIO_ERRINFO_H__

#include <string>

namespace lcy {
namespace asio {

#define LCY_ASIO_ERRINFO_MAP(XX)                        						\
    XX ( 0, 		SUCCESS, 			"Success" )								\
    XX ( -1001, 	EFDHUP, 			"Descriptor hang up" )					\
    XX ( -1002, 	EOPCANCELED, 		"Operation canceled" )					\
    XX ( -1003, 	EOPEXISTS, 			"Operation already exists" )			\


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
