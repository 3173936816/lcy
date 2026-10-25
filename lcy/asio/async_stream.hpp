#ifndef __LCY_ASIO_ASYNC_STREAM_HPP__
#define __LCY_ASIO_ASYNC_STREAM_HPP__

#include <memory>

#include "lcy/system/error.hpp"

namespace lcy
{
namespace asio
{

namespace detail
{

template <
	typename StreamSocket,
	typename MutableBufferView,
	typename Function
>
class async_read_state :
	public std::enable_shared_from_this<
		async_read_state<
			StreamSocket,
			MutableBufferView,
			Function
		>
	>
{
public:
	async_read_state(StreamSocket& stream_socket,
					 MutableBufferView buffer_view,
					 Function func) :
		stream_socket_(&stream_socket),
		buffer_view_(buffer_view),
		func_(std::move(func)),
		total_bytes_transferred_(0) {}

	void do_read()
	{
		auto self = this->shared_from_this();
		stream_socket_->async_read_some(buffer_view_,
				[this, self](const system::error_code& ec, std::size_t bytes_transferred) {
			if ( ec ) {
				func_(ec, total_bytes_transferred_);
				return;
			}

			total_bytes_transferred_ +=  bytes_transferred;

			if ( bytes_transferred >= buffer_view_.length() ) {
				func_(ec, total_bytes_transferred_);
				return;
			}

			buffer_view_ = buffer_view_ + bytes_transferred;
			do_read();
		});
	}

private:
	StreamSocket* stream_socket_;
	MutableBufferView buffer_view_;
	Function func_;
	std::size_t total_bytes_transferred_;
};

template <
	typename StreamSocket,
	typename ConstBufferView,
	typename Function
>
class async_write_state :
	public std::enable_shared_from_this<
		async_write_state<
			StreamSocket,
			ConstBufferView,
			Function
		>
	>
{
public:
	async_write_state(StreamSocket& stream_socket,
					  ConstBufferView buffer_view,
					  Function func) :
		stream_socket_(&stream_socket),
		buffer_view_(buffer_view),
		func_(std::move(func)),
		total_bytes_transferred_(0) {}

	void do_write()
	{
		auto self = this->shared_from_this();
		stream_socket_->async_write_some(buffer_view_,
				[this, self](const system::error_code& ec, std::size_t bytes_transferred) {
			if ( ec ) {
				func_(ec, total_bytes_transferred_);
				return;
			}

			total_bytes_transferred_ +=  bytes_transferred;

			if ( bytes_transferred >= buffer_view_.length() ) {
				func_(ec, total_bytes_transferred_);
				return;
			}

			buffer_view_ = buffer_view_ + bytes_transferred;
			do_write();
		});
	}

private:
	StreamSocket* stream_socket_;
	ConstBufferView buffer_view_;
	Function func_;
	std::size_t total_bytes_transferred_;
};

}	// namespace detail

template <
	typename StreamSocket,
	typename MutableBufferView,
	typename Function
>
void async_read(StreamSocket& stream_socket,
	MutableBufferView buffer_view, Function func)
{
	std::make_shared<detail::async_read_state<
		StreamSocket,
		MutableBufferView,
		Function
	> >(stream_socket, buffer_view, std::move(func))->do_read();
}

template <
	typename StreamSocket,
	typename ConstBufferView,
	typename Function
>
void async_write(StreamSocket& stream_socket,
	ConstBufferView buffer_view, Function func)
{
	std::make_shared<detail::async_write_state<
		StreamSocket,
		ConstBufferView,
		Function
	> >(stream_socket, buffer_view, std::move(func))->do_write();
}

}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_ASYNC_STREAM_HPP__
