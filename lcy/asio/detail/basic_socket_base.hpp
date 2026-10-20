#ifndef __LCY_ASIO_DETAIL_BASIC_SOCKET_BASE_HPP__
#define __LCY_ASIO_DETAIL_BASIC_SOCKET_BASE_HPP__

#include <functional>

#include "lcy/noncopyable.hpp"
#include "lcy/asio/detail/error.hpp"
#include "lcy/asio/detail/throw_asio_exception.hpp"

namespace lcy
{
namespace asio
{
namespace detail
{

template <
	typename Executor,
	typename Protocol,
	typename Endpoint,
	typename Service
>
class basic_socket_base :
	private noncopyable
{
public:
	typedef typename Service::accept_func_type accept_func_type;
	typedef typename Service::connect_func_type connect_func_type;
	typedef typename Service::read_func_type read_func_type;
	typedef typename Service::write_func_type write_func_type;

public:
	basic_socket_base(Executor& executor) :
		executor_(executor), service_(use_service<Service>(executor))
	{
		service_.construct(impl_);
	}

	~basic_socket_base()
	{
		service_.destroy(impl_);
	}

	bool is_open()
	{
		return service_.is_open(impl_);
	}

	void open(Protocol proto, lcy::system::error_code& ec)
	{
		service_.open(impl_, proto, ec);
	}

	void close(lcy::system::error_code& ec)
	{
		service_.close(impl_, ec);
	}

	void cancel(lcy::system::error_code& ec)
	{
		service_.cancel(impl_, ec);
	}

	void bind(Endpoint endpoint, lcy::system::error_code& ec)
	{
		service_.bind(impl_, endpoint, ec);
	}

	void connect(Endpoint endpoint, lcy::system::error_code& ec)
	{
		service_.connect(impl_, endpoint, ec);
	}

	template <typename MutableBufferView>
	std::size_t receive(MutableBufferView buffer_view, lcy::system::error_code& ec)
	{
		return service_.receive(impl_, buffer_view, ec);
	}

	template <typename ConstBufferView>
	std::size_t send(ConstBufferView buffer_view, lcy::system::error_code& ec)
	{
		return service_.send(impl_, buffer_view, ec);
	}

	template <typename SocketOption>
	void set_option(SocketOption opt, lcy::system::error_code& ec)
	{
		service_.set_option(impl_, opt, ec);
	}

	template <typename SocketOption>
	void get_option(SocketOption& opt, lcy::system::error_code& ec)
	{
		service_.get_option(impl_, opt, ec);
	}

	std::size_t available(lcy::system::error_code& ec)
	{
		return service_.available(impl_, ec);
	}

	void local_endpoint(Endpoint& endpoint, lcy::system::error_code& ec)
	{
		service_.local_endpoint(impl_, endpoint, ec);
	}

	void remote_endpoint(Endpoint& endpoint, lcy::system::error_code& ec)
	{
		service_.remote_endpoint(impl_, endpoint, ec);
	}

	void open(Protocol proto)
	{
		lcy::system::error_code ec;
		service_.open(impl_, proto, ec);
		if ( ec ) {
			throw_asio_exception(ec, "basic_socket_base::open");
		}
	}

	void close()
	{
		lcy::system::error_code ignored_ec;
		service_.close(impl_, ignored_ec);
	}

	void bind(Endpoint endpoint)
	{
		lcy::system::error_code ec;
		service_.bind(impl_, endpoint, ec);
		if ( ec ) {
			throw_asio_exception(ec, "basic_socket_base::bind");
		}
	}

	void connect(Endpoint endpoint)
	{
		lcy::system::error_code ec;
		service_.connect(impl_, endpoint, ec);
		if ( ec ) {
			throw_asio_exception(ec, "basic_socket_base::connect");
		}
	}

	template <typename MutableBufferView>
	std::size_t receive(MutableBufferView buffer_view)
	{
		lcy::system::error_code ec;
		std::size_t transferred_bytes = service_.receive(impl_, buffer_view, ec);
		if ( ec ) {
			throw_asio_exception(ec, "basic_socket_base::receive");
		}

		return transferred_bytes;
	}

	template <typename ConstBufferView>
	std::size_t send(ConstBufferView buffer_view)
	{
		lcy::system::error_code ec;
		std::size_t transferred_bytes = service_.send(impl_, buffer_view, ec);
		if ( ec ) {
			throw_asio_exception(ec, "basic_socket_base::send");
		}

		return transferred_bytes;
	}

	template <typename SocketOption>
	void set_option(SocketOption opt)
	{
		lcy::system::error_code ec;
		service_.set_option(impl_, opt, ec);
		if ( ec ) {
			throw_asio_exception(ec, "basic_socket_base::set_option");
		}
	}

	template <typename SocketOption>
	void get_option(SocketOption& opt)
	{
		lcy::system::error_code ec;
		service_.get_option(impl_, opt, ec);
		if ( ec ) {
			throw_asio_exception(ec, "basic_socket_base::get_option");
		}
	}

	std::size_t available()
	{
		lcy::system::error_code ignored_ec;
		return service_.available(impl_, ignored_ec);
	}

	void local_endpoint(Endpoint& endpoint)
	{
		lcy::system::error_code ec;
		service_.local_endpoint(impl_, endpoint, ec);
		if ( ec ) {
			throw_asio_exception(ec, "basic_socket_base::remote_endpoint");
		}
	}

	void remote_endpoint(Endpoint& endpoint)
	{
		lcy::system::error_code ec;
		service_.remote_endpoint(impl_, endpoint, ec);
		if ( ec ) {
			throw_asio_exception(ec, "basic_socket_base::remote_endpoint");
		}
	}

	void cancel()
	{
		lcy::system::error_code ignored_ec;
		service_.cancel(impl_, ignored_ec);
	}

	void async_connect(Endpoint endpoint, connect_func_type func)
	{
		service_.async_connect(impl_, endpoint, std::move(func));
	}

	template <typename MutableBufferView>
	void async_receive(MutableBufferView buffer_view, read_func_type func)
	{
		service_.async_receive(impl_, buffer_view, std::move(func));
	}

	template <typename ConstBufferView>
	void async_send(ConstBufferView buffer_view, write_func_type func)
	{
		service_.async_send(impl_, buffer_view, std::move(func));
	}

	Executor& get_executor()
	{
		return executor_;
	}

protected:
	typedef typename Service::socket_impl socket_impl;

	Executor& executor_;
	socket_impl impl_;
	Service& service_;
};

}	// namespace detail
}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_DETAIL_BASIC_SOCKET_BASE_HPP__
