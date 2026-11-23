#ifndef __lCY_ASIO_DETAIL_DATAGRAM_SOCKET_SERVICE_HPP__
#define __lCY_ASIO_DETAIL_DATAGRAM_SOCKET_SERVICE_HPP__

#include "lcy/asio/detail/reactor.hpp"
#include "lcy/asio/detail/scheduler.hpp"
#include "lcy/asio/detail/socket_service_base.hpp"
#include "lcy/asio/detail/service_registry_base.hpp"

namespace lcy
{
namespace asio
{
namespace detail
{

class datagram_socket_service :
	public socket_service_base,
	public service_registry_base<
		datagram_socket_service
	>
{
public:
	explicit datagram_socket_service(scheduler& sch) :
		socket_service_base(sch) {}

	template <
		typename Endpoint,
		typename MutableBufferView
	>
	std::size_t receive_from(socket_impl& impl,
		Endpoint& endpoint, MutableBufferView view,
		lcy::system::error_code& ec)
	{
		if ( !is_open(impl) ) {
            ec = lcy::system::error_code(
                lcy::system::error::bad_file_descriptor);
            return 0;
        }

		ssize_t transferred_bytes = 0;

		for ( ;; ) {
			transferred_bytes = socket_ops::recvfrom(impl.sockfd_,
				endpoint, view, ec);

			if ( transferred_bytes >= 0 ) {
				break;
			}
			
			if ( ec == lcy::system::error::resource_unavailable_try_again ) {
				if ( socket_ops::poll_read(impl.sockfd_, ec) ) {
					return 0;
				}
				continue;
			}

			return 0;
		}

		return static_cast<std::size_t>(transferred_bytes);
	}

	template <
		typename Endpoint,
		typename ConstBufferView
	>
	std::size_t send_to(socket_impl& impl,
		Endpoint endpoint, ConstBufferView view,
		lcy::system::error_code& ec)
	{
		if ( !is_open(impl) ) {
            ec = lcy::system::error_code(
                lcy::system::error::bad_file_descriptor);
            return 0;
        }

		ssize_t transferred_bytes = 0;

		for ( ;; ) {
			transferred_bytes = socket_ops::sendto(impl.sockfd_,
				endpoint, view, ec);

			if ( transferred_bytes >= 0 ) {
				break;
			}

			if ( ec == lcy::system::error::resource_unavailable_try_again ) {
				if ( socket_ops::poll_write(impl.sockfd_, ec) ) {
					return 0;
				}
				continue;
			}

			return 0;
		}

		return static_cast<std::size_t>(transferred_bytes);
	}

	template <
		typename Endpoint,
		typename MutableBufferView
	>
	void async_receive_from(socket_impl& impl,
		Endpoint& endpoint, MutableBufferView view,
		read_func_type func)
	{
		if ( !is_open(impl) ) {
            lcy::system::error_code ec = lcy::system::error_code(
                lcy::system::error::bad_file_descriptor);
			post_func_to_scheduler(std::move(func), ec, 0);
            return;
        }

		receive_from_operation<Endpoint, MutableBufferView> receive_from_op(this,
			impl, endpoint, view, std::move(func));

		reactor_->start_op(impl.sockfd_,
			impl.data_, reactor::event_type::read,
				operation(std::move(receive_from_op)), true);
	}

	template <
		typename Endpoint,
		typename ConstBufferView
	>
	void async_send_to(socket_impl& impl,
		Endpoint endpoint, ConstBufferView view,
		write_func_type func)
	{
		if ( !is_open(impl) ) {
            lcy::system::error_code ec = lcy::system::error_code(
                lcy::system::error::bad_file_descriptor);
			post_func_to_scheduler(std::move(func), ec, 0);
            return;
        }

		send_to_operation<Endpoint, ConstBufferView> send_to_op(this,
			impl, endpoint, view, std::move(func));

		reactor_->start_op(impl.sockfd_,
			impl.data_, reactor::event_type::write,
				operation(std::move(send_to_op)), true);
	}

	void cleanup() override
	{
		// nothing to do
	}
};

}	// namespace detail
}	// namespace asio
}	// namespace lcy

#endif	// __lCY_ASIO_DETAIL_DATAGRAM_SOCKET_SERVICE_HPP__
