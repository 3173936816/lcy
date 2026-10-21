#ifndef __LCY_ASIO_DETAIL_BASIC_SOCKET_OPTION_HPP__
#define __LCY_ASIO_DETAIL_BASIC_SOCKET_OPTION_HPP__

#include <cstdint>

namespace lcy
{
namespace asio
{
namespace detail
{

template <int Level, int Name, typename Value>
class basic_socket_option
{
public:
	basic_socket_option() : value_() {}

	explicit basic_socket_option(Value value) :
        value_(value) {}

	int level() const
    { 
        return Level;
    }

	int name() const
    {
        return Name; 
    }

	void* data()
    { 
        return &value_;
    }

	std::size_t data_len() const
    { 
        return sizeof(Value);
    }

	Value& value()
    { 
        return value_;
    }

private:
	Value value_;
};

}    // namespace detail
}    // namespace asio
}    // namespace lcy

#endif  // __LCY_ASIO_DETAIL_BASIC_SOCKET_OPTION_HPP__