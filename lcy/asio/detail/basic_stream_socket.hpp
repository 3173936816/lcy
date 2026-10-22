#ifndef __LCY_ASIO_DETAIL_BASIC_STREAM_SOCKET_HPP__
#define __LCY_ASIO_DETAIL_BASIC_STREAM_SOCKET_HPP__

#include "lcy/asio/detail/basic_socket_base.hpp"
#include "lcy/asio/detail/stream_socket_service.hpp"

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
class basic_stream_socket :
	public basic_socket_base<
		Executor,
		Protocol,
		Endpoint,
		stream_socket_service
	>
{
public:
	typedef basic_socket_base<
		Executor,
		Protocol,
		Endpoint,
		stream_socket_service
	> base_type;

	typedef Executor executor_type;
    typedef Protocol protocol_type;
    typedef Endpoint endpoint_type;

	typedef typename base_type::accept_func_type accept_func_type;
	typedef typename base_type::connect_func_type connect_func_type;
	typedef typename base_type::read_func_type read_func_type;
	typedef typename base_type::write_func_type write_func_type;

public:
	basic_stream_socket(executor_type& executor) :
		base_type(executor) {}

	void listen(int backlog, lcy::system::error_code& ec)
	{
		base_type::service_.listen(base_type::impl_, backlog, ec);
	}

	void accept(basic_stream_socket& target_socket,
		endpoint_type& endpoint, lcy::system::error_code& ec)
	{
		base_type::service_.accept(base_type::impl_,
			target_socket.impl_, endpoint, ec);
	}

	template <typename MutableBufferView>
	std::size_t read_some(MutableBufferView buffer_view, lcy::system::error_code& ec)
	{
		return base_type::service_.read_some(base_type::impl_, buffer_view, ec);
	}

	template <typename ConstBufferView>
	std::size_t write_some(ConstBufferView buffer_view, lcy::system::error_code& ec)
	{
		return base_type::service_.write_some(base_type::impl_, buffer_view, ec);
	}

	void listen(int backlog)
	{
		lcy::system::error_code ec;
		base_type::service_.listen(base_type::impl_, backlog, ec);
		if ( ec ) {
			throw_asio_exception(ec, "basic_stream_socket::listen");
		}
	}

	void accept(basic_stream_socket& target_socket, endpoint_type& endpoint)
	{
		lcy::system::error_code ec;
		base_type::service_.accept(base_type::impl_, target_socket.impl_, endpoint, ec);
		if ( ec ) {
			throw_asio_exception(ec, "basic_stream_socket::accept");
		}
	}

	template <typename MutableBufferView>
	std::size_t read_some(MutableBufferView buffer_view)
	{
		lcy::system::error_code ec;
		std::size_t transferred_bytes =
			base_type::service_.read_some(base_type::impl_, buffer_view, ec);
		if ( ec ) {
			throw_asio_exception(ec, "basic_stream_socket::read_some");
		}

		return transferred_bytes;
	}

	template <typename ConstBufferView>
	std::size_t write_some(ConstBufferView buffer_view)
	{
		lcy::system::error_code ec;
		std::size_t transferred_bytes =
			base_type::service_.write_some(base_type::impl_, buffer_view, ec);
		if ( ec ) {
			throw_asio_exception(ec, "basic_stream_socket::write_some");
		}
	
		return transferred_bytes;
	}

	void async_accept(basic_stream_socket& target_socket,
		endpoint_type& endpoint, accept_func_type func)
	{
		base_type::service_.async_accept(base_type::impl_,
			target_socket.impl_, endpoint, std::move(func));
	}

	template <typename MutableBufferView>
	void async_read_some(MutableBufferView buffer_view, read_func_type func)
	{
		base_type::service_.async_read_some(base_type::impl_,
			buffer_view, std::move(func));
	}

	template <typename ConstBufferView>
	void async_write_some(ConstBufferView buffer_view, write_func_type func)
	{
		base_type::service_.async_write_some(base_type::impl_,
			buffer_view, std::move(func));
	}
};

}	// namespace detail
}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_DETAIL_BASIC_STREAM_SOCKET_HPP__
