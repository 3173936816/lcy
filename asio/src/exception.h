#ifndef __LCY_ASIO_EXCEPTION_H__
#define __LCY_ASIO_EXCEPTION_H__

#include <exception>
#include <string>

namespace lcy {
namespace asio {

class LcyAsioException :
	public std::exception
{
public:
	LcyAsioException(const std::string& exmsg = "");
	~LcyAsioException();

	const char* what() const noexcept override;

private:
	std::string exmsg_;
};

}	// namespace asio
}	// namespace lcy

#endif // __LCY_ASIO_EXCEPTION_H__
