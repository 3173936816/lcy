#ifndef __LCY_ASIO_DETAIL_STREAM_SOCKET_SERVICE_HPP__
#define __LCY_ASIO_DETAIL_STREAM_SOCKET_SERVICE_HPP__

#include "lcy/asio/detail/socket_service_base.hpp"
#include "lcy/asio/detail/service_registry_base.hpp"

namespace lcy
{
namespace asio
{
namespace detail
{

class stream_socket_service :
	public socket_service_base,
	public service_registry_base<
		stream_socket_service
	>
{
public:
	stream_socket_service(reactor& r) :
		socket_service_base(r) {}

	void listen(socket_impl& impl,
		int backlog, lcy::system::error_code& ec)
	{
		if ( !is_open(impl) ) {
			ec = lcy::system::error_code(
				lcy::system::error::bad_file_descriptor);
			return;
		}

		socket_ops::listen(impl.sockfd_, backlog, ec);
	}

	template <typename Endpoint>
	void accept(socket_impl& source_impl,
		socket_impl& target_impl, Endpoint& endpoint,
		lcy::system::error_code& ec)
	{
		if ( !is_open(source_impl) ) {
			ec = lcy::system::error_code(
				lcy::system::error::bad_file_descriptor);
			return;
		}

		if ( is_open(target_impl) ) {
			ec = make_asio_error_code(
				lcy::asio::error::already_open);	
			return;
		}

		int new_sockfd = -1;
		for ( ;; ) {
        	new_sockfd = socket_ops::accept(source_impl.sockfd_, endpoint, ec);

        	if ( new_sockfd != -1 ) {
        	    break;
			}

        	if ( ec == lcy::system::error::resource_unavailable_try_again ) {
        	    if ( socket_ops::select_read(source_impl.sockfd_, ec) ) {
        	        return;
				}
        	    continue;
        	}

        	return;
    	}

		assign(target_impl, new_sockfd, ec);
	}

	template <typename MutableBufferView>
	std::size_t read_some(socket_impl& impl,
		MutableBufferView buffer_view, lcy::system::error_code& ec)
	{
		return receive(impl, buffer_view, ec);
	}

	template <typename ConstBufferView>
	std::size_t write_some(socket_impl& impl,
		ConstBufferView buffer_view, lcy::system::error_code& ec)
	{
		return send(impl, buffer_view, ec);
	}

	template <typename Endpoint>
	void async_accept(socket_impl& source_impl,
		socket_impl& target_impl, Endpoint& endpoint,
		accept_func_type func)
	{
		if ( !is_open(source_impl) ) {
			lcy::system::error_code ec(
				lcy::system::error::bad_file_descriptor);
			post_func_to_scheduler(std::move(func), ec);
			return;
		}
	
		if ( is_open(target_impl) ) {
			lcy::system::error_code ec = 
				make_asio_error_code(lcy::asio::error::already_open);	
			post_func_to_scheduler(std::move(func), ec);
			return;
		}
	
		accept_operation<Endpoint> accept_op(this,
			source_impl, target_impl, endpoint, std::move(func));
	
		reactor_->start_op(source_impl.sockfd_,
			source_impl.data_, reactor::event_type::read,
				operation(std::move(accept_op)));
	}

	template <typename MutableBufferView>
	void async_read_some(socket_impl& impl,
		MutableBufferView buffer_view, read_func_type func)
	{
		async_receive(impl, buffer_view, std::move(func));
	}

	template <typename ConstBufferView>
	void async_write_some(socket_impl& impl,
		ConstBufferView buffer_view, write_func_type func)
	{
		async_send(impl, buffer_view, std::move(func));
	}

	void cleanup() override
	{
		// nothing to do
	}
};
	
}	// namespace detail
}	// namespaace asio
}	// namespace lcy

#endif	// __LCY_ASIO_DETAIL_STREAM_SOCKET_SERVICE_HPP__
