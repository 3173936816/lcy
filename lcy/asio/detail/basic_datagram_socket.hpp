#ifndef __LCY_ASIO_DETAIL_BASIC_DATAGRAM_SOCKET_HPP__
#define __LCY_ASIO_DETAIL_BASIC_DATAGRAM_SOCKET_HPP__

#include "lcy/asio/detail/basic_socket_base.hpp"
#include "lcy/asio/detail/datagram_socket_service.hpp"

namespace lcy
{
namespace asio
{
namespace detail
{

template <
	typename Executor,
	typename Protocol,
	typename Endpoint
>
class basic_datagram_socket :
	public basic_socket_base<
		Executor,
		Protocol,
		Endpoint,
		datagram_socket_service
	>
{
public:
	typedef basic_socket_base<
		Executor,
		Protocol,
		Endpoint,
		datagram_socket_service
	> base_type;

	typedef Executor executor_type;
    typedef Protocol protocol_type;
    typedef Endpoint endpoint_type;

	typedef typename base_type::connect_func_type connect_func_type;
	typedef typename base_type::read_func_type read_func_type;
	typedef typename base_type::write_func_type write_func_type;

public:
	explicit basic_datagram_socket(executor_type& executor) :
		base_type(executor) {}

	template <typename MutableBufferView>
	std::size_t receive_from(endpoint_type& endpoint,
		MutableBufferView buffer_view, lcy::system::error_code& ec)
	{
		return base_type::service_.receive_from(base_type::impl_,
			endpoint, buffer_view, ec);
	}

	template <typename ConstBufferView>
	std::size_t send_to(endpoint_type endpoint,
		ConstBufferView buffer_view, lcy::system::error_code& ec)
	{
		return base_type::service_.send_to(base_type::impl_,
			endpoint, buffer_view, ec);
	}
	
	template <typename MutableBufferView>
	std::size_t receive_from(endpoint_type& endpoint, MutableBufferView buffer_view)
	{
		lcy::system::error_code ec;
		std::size_t transferred_bytes =
			base_type::service_.receive_from(base_type::impl_,
				endpoint, buffer_view, ec);
		if ( ec ) {
			throw_asio_exception(ec, "basic_datagram_socket::receive_from");
		}

		return transferred_bytes;
	}

	template <typename ConstBufferView>
	std::size_t send_to(endpoint_type endpoint, ConstBufferView buffer_view)
	{
		lcy::system::error_code ec;
		std::size_t transferred_bytes =
			base_type::service_.send_to(base_type::impl_,
				endpoint, buffer_view, ec);
		if ( ec ) {
			throw_asio_exception(ec, "basic_datagram_socket::send_to");
		}

		return transferred_bytes;
	}

	template <typename MutableBufferView>
	void async_receive_from(endpoint_type& endpoint,
		MutableBufferView buffer_view, read_func_type func)
	{
		base_type::service_.async_receive_from(base_type::impl_,
			endpoint, buffer_view, std::move(func));
	}
	
	template <typename ConstBufferView>
	void async_send_to(endpoint_type endpoint,
		ConstBufferView buffer_view, write_func_type func)
	{
		base_type::service_.async_send_to(base_type::impl_,
			endpoint, buffer_view, std::move(func));
	}
};

}	// namespace detail
}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_DETAIL_BASIC_DATAGRAM_SOCKET_HPP__
