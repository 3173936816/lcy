#ifndef __LCY_ASIO_STREAM_WRITE_HPP__
#define __LCY_ASIO_STREAM_WRITE_HPP__

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
					  ConstBufferView view,
					  Function func) :
		stream_socket_(&stream_socket),
		view_(view),
		func_(std::move(func)),
		total_bytes_transferred_(0) {}

	void do_write()
	{
		auto self = this->shared_from_this();
		auto on_write_func = [this, self](lcy::system::error_code ec,
			std::size_t bytes_transferred)
		{
			if ( ec ) {
				func_(ec, total_bytes_transferred_);
				return;
			}

			total_bytes_transferred_ +=  bytes_transferred;

			if ( bytes_transferred >= view_.length() ) {
				func_(ec, total_bytes_transferred_);
				return;
			}

			view_ += bytes_transferred;
			do_write();
		};

		stream_socket_->async_write_some(view_, std::move(on_write_func));
	}

private:
	StreamSocket* stream_socket_;
	ConstBufferView view_;
	Function func_;
	std::size_t total_bytes_transferred_;
};

}   // namespace detail

template <
	typename StreamSocket,
	typename ConstBufferView
>
std::size_t write(StreamSocket& stream_socket,
    ConstBufferView view, lcy::system::error_code& ec)
{
    std::size_t total = 0;

    for ( ;; ) {
        std::size_t n = stream_socket.write_some(view, ec);

        if ( ec ) {
			break;
        }

        total += n;
        if ( n == view.length() ) {
            break;
        }

        view += n;
    }

	return total;
}

template <
	typename StreamSocket,
	typename ConstBufferView
>
std::size_t write(StreamSocket& stream_socket, ConstBufferView view)
{
    lcy::system::error_code ec;
    std::size_t n = write(stream_socket, view, ec);
    if ( ec ) {
        detail::throw_asio_exception(ec, "lcy::asio::write");
    }

    return n;
}

template <
	typename StreamSocket,
	typename ConstBufferView,
	typename Function
>
void async_write(StreamSocket& stream_socket,
	ConstBufferView view, Function func)
{
	std::make_shared<detail::async_write_state<
		StreamSocket,
		ConstBufferView,
		Function
	> >(stream_socket, view, std::move(func))->do_write();
}

}   // namespace asio
}   // namespace lcy

#endif  // __LCY_ASIO_STREAM_WRITE_HPP__