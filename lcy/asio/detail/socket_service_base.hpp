#ifndef __LCY_ASIO_DETAIL_SOCKET_SERVICE_BASE_HPP__
#define __LCY_ASIO_DETAIL_SOCKET_SERVICE_BASE_HPP__

#include <memory>

#include "lcy/asio/detail/error.hpp"
#include "lcy/asio/detail/reactor.hpp"
#include "lcy/asio/detail/socket_ops.hpp"

namespace lcy
{
namespace asio
{
namespace detail
{

class socket_service_base
{
public:
	typedef std::function<
		void (
			lcy::system::error_code ec
		)
	> accept_func_type;

	typedef accept_func_type connect_func_type;

	typedef std::function<
		void (
			lcy::system::error_code ec,
			std::size_t transferred_bytes
		)
	> read_func_type;

	typedef read_func_type write_func_type;

public:
	typedef reactor::internal_descriptor_state internal_descriptor_state;

	struct socket_impl
	{
		int sockfd_;
		internal_descriptor_state* data_;
	};

protected:
	template <typename Function>
	class post_operation;

	template <typename Endpoint>
	class accept_operation;

	template <typename Endpoint>
	class connect_operation;

	template <typename MutableBufferView>
	class receive_operation;

	template <typename ConstBufferView>
	class send_operation;

	template <
		typename Endpoint,
		typename MutableBufferView
	>
	class receive_from_operation;

	template <
		typename Endpoint,
		typename ConstBufferView
	>
	class send_to_operation;

public:
	typedef int native_handle_type;

	explicit socket_service_base(reactor& r) :
		reactor_(&r) {}

	void construct(socket_impl& impl)
	{
		impl.sockfd_ = -1;
		impl.data_ = 0;
	}

	void destroy(socket_impl& impl)
	{
		lcy::system::error_code ignore_ec;
		this->close(impl, ignore_ec);
	}

	native_handle_type native_handle(socket_impl& impl)
	{
		return impl.sockfd_;
	}

	bool is_open(const socket_impl& impl) const
	{
		return impl.sockfd_ != -1;
	}

	template <typename Protocol>
	void open(socket_impl& impl, Protocol proto,
		lcy::system::error_code& ec)
	{
		if ( is_open(impl) ) {
			ec = make_asio_error_code(
				lcy::asio::error::already_open);	
			return;
		}		

		int sockfd = socket_ops::open(proto, ec);
		if ( sockfd == -1 ) {
			return;
		}

		assign(impl, sockfd, ec);
	}

	void close(socket_impl& impl, lcy::system::error_code& ec)
	{
		if ( is_open(impl) ) {
			reactor_->cancel_ops(impl.sockfd_,
				impl.data_, reactor::event_type::read | reactor::event_type::write);
			reactor_->remove_internal_descriptor_state(impl.sockfd_, impl.data_);
			reactor_->free_internal_descriptor_state(impl.data_);
			socket_ops::close(impl.sockfd_, ec);
		}

		impl.sockfd_ = -1;
		impl.data_ = 0;
	}

	template <typename Endpoint>
	void bind(socket_impl& impl, Endpoint endpoint,
		lcy::system::error_code& ec)
	{
		if ( !is_open(impl) ) {
			ec = lcy::system::error_code(
				lcy::system::error::bad_file_descriptor);
			return;
		}

		socket_ops::bind(impl.sockfd_, endpoint, ec);
	}

	template <typename Endpoint>
	void connect(socket_impl& impl,
		Endpoint endpoint, lcy::system::error_code& ec)
	{
		if ( !is_open(impl) ) {
			ec = lcy::system::error_code(
				lcy::system::error::bad_file_descriptor);
			return;
		}

		if ( socket_ops::connect(impl.sockfd_, endpoint, ec) ) {
			if ( ec != lcy::system::error::operation_in_progress ) {
				return;
			}

			socket_ops::poll_connect(impl.sockfd_, ec);
		}
	}

	template <typename MutableBufferView>
	std::size_t receive(socket_impl& impl,
		MutableBufferView buffer_view, lcy::system::error_code& ec)
	{
		if ( !is_open(impl) ) {
            ec = lcy::system::error_code(
                lcy::system::error::bad_file_descriptor);
            return 0;
        }

		ssize_t transferred_bytes = 0;

		for ( ;; ) {
			transferred_bytes = socket_ops::recv(impl.sockfd_, buffer_view, ec);

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

	template <typename ConstBufferView>
	std::size_t send(socket_impl& impl,
		ConstBufferView buffer_view, lcy::system::error_code& ec)
	{
		if ( !is_open(impl) ) {
            ec = lcy::system::error_code(
                lcy::system::error::bad_file_descriptor);
            return 0;
        }

		ssize_t transferred_bytes = 0;

		for ( ;; ) {
			transferred_bytes = socket_ops::send(impl.sockfd_, buffer_view, ec);

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

	template <typename SocketOption>
	void set_option(socket_impl& impl,
		SocketOption opt, lcy::system::error_code& ec)
	{
		if ( !is_open(impl) ) {
			ec = lcy::system::error_code(
				lcy::system::error::bad_file_descriptor);
			return;
		}

		socket_ops::setsockopt(impl.sockfd_,
			opt.level(), opt.name(), opt.data(), opt.data_len(), ec);
	}

	template <typename SocketOption>
	void get_option(socket_impl& impl,
		SocketOption& opt, lcy::system::error_code& ec)
	{
		if ( !is_open(impl) ) {
			ec = lcy::system::error_code(
				lcy::system::error::bad_file_descriptor);
			return;
		}

		socklen_t optlen = static_cast<socklen_t>(opt.data_len());
		socket_ops::getsockopt(impl.sockfd_,
			opt.level(), opt.name(), opt.data(), &optlen, ec);
	}

	std::size_t available(socket_impl& impl, lcy::system::error_code& ec)
	{
		if ( !is_open(impl) ) {
			ec = lcy::system::error_code(
				lcy::system::error::bad_file_descriptor);
			return 0;
		}
	
		int available_bytes = 0;
		if ( socket_ops::ioctl(impl.sockfd_, FIONREAD, ec, &available_bytes) ) {
			return 0;
		}

		return static_cast<std::size_t>(available_bytes);
	}

	template <typename Endpoint>
	void local_endpoint(socket_impl& impl,
		Endpoint& endpoint, lcy::system::error_code& ec)
	{
		if ( !is_open(impl) ) {
			ec = lcy::system::error_code(
				lcy::system::error::bad_file_descriptor);
			return;
		}

		socket_ops::getsockname(impl.sockfd_, endpoint, ec);
	}

	template <typename Endpoint>
	void remote_endpoint(socket_impl& impl,
		Endpoint& endpoint, lcy::system::error_code& ec)
	{
		if ( !is_open(impl) ) {
			ec = lcy::system::error_code(
				lcy::system::error::bad_file_descriptor);
			return;
		}

		socket_ops::getpeername(impl.sockfd_, endpoint, ec);
	}

	void cancel(socket_impl& impl, lcy::system::error_code& ec)
	{
		if ( !is_open(impl) ) {
			ec = lcy::system::error_code(
				lcy::system::error::bad_file_descriptor);
			return;
		}

		reactor_->cancel_ops(impl.sockfd_,
			impl.data_, reactor::event_type::read | reactor::event_type::write);
	}

	template <typename Endpoint>
	void async_connect(socket_impl& impl,
		Endpoint endpoint, connect_func_type func)
	{
		if ( !is_open(impl) ) {
			lcy::system::error_code ec(
				lcy::system::error::bad_file_descriptor);
			post_func_to_scheduler(std::move(func), ec);
			return;
		}
	
		lcy::system::error_code ec;
		if ( socket_ops::connect(impl.sockfd_, endpoint, ec) ) {
			if ( ec != lcy::system::error::operation_in_progress ) {
				post_func_to_scheduler(std::move(func), ec);
				return;
			}
	
			connect_operation<Endpoint> connect_op(this, impl, std::move(func));
	
			reactor_->start_op(impl.sockfd_,
				impl.data_, reactor::event_type::write,
					operation(std::move(connect_op)), false);
	
			return;
		}
	
		post_func_to_scheduler(std::move(func), ec);
	}

	template <typename MutableBufferView>
	void async_receive(socket_impl& impl,
		MutableBufferView buffer_view, read_func_type func)
	{
		if ( !is_open(impl) ) {
            lcy::system::error_code ec = lcy::system::error_code(
                lcy::system::error::bad_file_descriptor);
			post_func_to_scheduler(std::move(func), ec, 0);
            return;
        }

		receive_operation<MutableBufferView> receive_op(this,
			impl, buffer_view, std::move(func));

		reactor_->start_op(impl.sockfd_,
			impl.data_, reactor::event_type::read,
				operation(std::move(receive_op)), true);
	}

	template <typename ConstBufferView>
	void async_send(socket_impl& impl,
		ConstBufferView buffer_view, write_func_type func)
	{
		if ( !is_open(impl) ) {
            lcy::system::error_code ec = lcy::system::error_code(
                lcy::system::error::bad_file_descriptor);
			post_func_to_scheduler(std::move(func), ec, 0);
            return;
        }

		send_operation<ConstBufferView> send_op(this,
			impl, buffer_view, std::move(func));

		reactor_->start_op(impl.sockfd_,
			impl.data_, reactor::event_type::write,
				operation(std::move(send_op)), true);
	}

protected:
	template <typename Function, typename ...Args>
	void post_func_to_scheduler(Function func,
		lcy::system::error_code ec, Args&&... args)
	{
		post_operation<Function> post_op(this,
			std::move(func), std::forward<Args>(args)...);
	
		operation op(std::move(post_op));
		op.set_errorc(ec);
	
		reactor_->post_to_scheduler(std::move(op));
	}

	void assign(socket_impl& impl, int sockfd, lcy::system::error_code& ec)
	{
		impl.sockfd_ = sockfd;
		impl.data_ = reactor_->alloc_internal_descriptor_state();
		int error = reactor_->register_internal_descriptor_state(impl.sockfd_, impl.data_);
		if ( error ) {
			lcy::system::error_code ignored_ec;
			close(impl, ignored_ec);
		}

		ec = lcy::system::error_code(error);
	}
	
protected:
	reactor* reactor_;
};

}	// namespace detail
}	// namespace asio
}	// namespace lcy

#include "lcy/asio/detail/impl/post_operation.ipp"
#include "lcy/asio/detail/impl/accept_operation.ipp"
#include "lcy/asio/detail/impl/connect_operation.ipp"
#include "lcy/asio/detail/impl/receive_operation.ipp"
#include "lcy/asio/detail/impl/send_operation.ipp"
#include "lcy/asio/detail/impl/receive_from_operation.ipp"
#include "lcy/asio/detail/impl/send_to_operation.ipp"

#endif	// __LCY_ASIO_DETAIL_SOCKET_SERVICE_BASE_HPP__
