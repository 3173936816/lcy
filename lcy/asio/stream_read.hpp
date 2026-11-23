#ifndef __LCY_ASIO_STREAM_READ_HPP__
#define __LCY_ASIO_STREAM_READ_HPP__

#include <memory>

#include "lcy/system/error.hpp"
#include "lcy/asio/detail/throw_asio_exception.hpp"

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
		auto on_read_func = [this, self](lcy::system::error_code ec,
			std::size_t bytes_transferred)
		{
			if ( ec ) {
				func_(ec, total_bytes_transferred_);
				return;
			}

			total_bytes_transferred_ +=  bytes_transferred;

			if ( bytes_transferred >= buffer_view_.length() ) {
				func_(ec, total_bytes_transferred_);
				return;
			}

			buffer_view_ += bytes_transferred;
			do_read();
		};

		stream_socket_->async_read_some(buffer_view_, std::move(on_read_func));
	}

private:
	StreamSocket* stream_socket_;
	MutableBufferView buffer_view_;
	Function func_;
	std::size_t total_bytes_transferred_;
};

}   // namespace detail

template <
    typename StreamSocket,
    typename MutableBufferView
>
std::size_t read(StreamSocket& stream_socket,
    MutableBufferView buffer_view, lcy::system::error_code& ec)
{
    std::size_t total = 0;

    for ( ;; ) {
        std::size_t n = stream_socket.read_some(buffer_view, ec);

        if ( ec ) {
            return total;
        }

        total += n;
        if ( n == buffer_view.length() ) {
            return total;
        }

        buffer_view += n;
    }
}

template <
    typename StreamSocket,
    typename MutableBufferView
>
std::size_t read(StreamSocket& stream_socket, MutableBufferView buffer_view)
{
    lcy::system::error_code ec;
    std::size_t n = read(stream_socket, buffer_view, ec);
    if ( ec ) {
        detail::throw_asio_exception(ec, "lcy::asio::read");
    }

    return n;
}

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

}   // namespace asio
}   // namespace lcy

#endif  // __LCY_ASIO_STREAM_READ_HPP__