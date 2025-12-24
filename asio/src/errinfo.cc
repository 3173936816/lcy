#include "lcy/asio/src/errinfo.h"

#include <errno.h>
#include <string.h>

namespace lcy {
namespace asio {

static std::string explain(int errcode)
{
#define XX(_code, _err, _info)	\
	if ( errcode == _code ) return _info;
	LCY_ASIO_ERRINFO_MAP(XX)
#undef XX
	return "Unknown error";
}

std::string errinfo(errcode_type errcode)
{
	if ( errcode <= 0 ) {
		return explain(errcode);
	}

	return ::strerror(errcode);
}

}	// namespace asio
}	// namespace lcy
