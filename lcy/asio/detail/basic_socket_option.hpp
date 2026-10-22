#ifndef __LCY_ASIO_DETAIL_BASIC_SOCKET_OPTION_HPP__
#define __LCY_ASIO_DETAIL_BASIC_SOCKET_OPTION_HPP__

#include <cstdint>

namespace lcy
{
namespace asio
{
namespace detail
{

template <int Level, int OptName, typename OptValue>
class basic_socket_option
{
public:
    typedef OptValue optval_type;

	basic_socket_option() : value_() {}

	explicit basic_socket_option(optval_type value) :
        value_(value) {}

	int level() const
    { 
        return Level;
    }

	int name() const
    {
        return OptName; 
    }

	void* data()
    { 
        return &value_;
    }

    const void* data() const
    { 
        return &value_;
    }

	std::size_t data_len() const
    { 
        return sizeof(optval_type);
    }

	optval_type& value()
    { 
        return value_;
    }

    const optval_type& value() const
    { 
        return value_;
    }

private:
	optval_type value_;
};

}    // namespace detail
}    // namespace asio
}    // namespace lcy

#endif  // __LCY_ASIO_DETAIL_BASIC_SOCKET_OPTION_HPP__