#include "asio/src/exception.h"

namespace lcy {
namespace asio {

LcyAsioException::LcyAsioException(const std::string& exmsg) :
	exmsg_(exmsg)
{
}

LcyAsioException::~LcyAsioException()
{
}

const char* LcyAsioException::what() const noexcept
{
	return exmsg_.c_str();
}

}	// namespace asio
}	// namespace lcy
