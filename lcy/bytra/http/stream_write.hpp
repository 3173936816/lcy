#ifndef __LCY_BYTRA_HTTP_STREAM_WRITE_HPP__
#define __LCY_BYTRA_HTTP_STREAM_WRITE_HPP__

#include <memory>
#include <string>
#include <utility>
#include <cstddef>
#include <functional>

#include "lcy/system/error.hpp"
#include "lcy/asio/stream_write.hpp"
#include "lcy/bytra/flat_buffer.hpp"
#include "lcy/bytra/http/error.hpp"
#include "lcy/bytra/http/request_parser.hpp"
#include "lcy/bytra/http/response_parser.hpp"
#include "lcy/bytra/http/detail/throw_http_exception.hpp"

namespace lcy
{
namespace bytra
{
namespace http
{

namespace detail
{

template <typename Function>
class async_write_header_state :
    public std::enable_shared_from_this<
        async_write_header_state<
            Function
        >
    >
{
public:
    async_write_header_state(std::string header, Function func) :
        header_(std::move(header)), func_(std::move(func)) {}

    template <typename StreamSocket>
    void do_write(StreamSocket& stream_socket)
    {
        auto self = this->shared_from_this();
        auto view = lcy::asio::buffer_view(
            header_.c_str(), header_.size());

        lcy::asio::async_write(stream_socket, view, std::bind(
            &async_write_header_state::on_write_func, self,
                std::placeholders::_1, std::placeholders::_2));
    }

private:
    void on_write_func(lcy::system::error_code ec,
        std::size_t bytes_transferred)
    {
        func_(ec, bytes_transferred);
    }

private:
    std::string header_;
    Function func_;
};

template <
    typename StreamSocket,
    typename Message,
    typename Function
>
class async_write_state :
    public std::enable_shared_from_this<
        async_write_state<
            StreamSocket,
            Message,
            Function
        >
    >
{
public:
    async_write_state(StreamSocket& stream_socket,
        Message& message, Function func) :
        stream_socket_(&stream_socket),
        message_(&message),
        header_(message.header_to_string()),
        func_(std::move(func)),
        header_bytes_transferred_(0) {}

    void do_write()
    {
        auto self = this->shared_from_this();
        auto view = lcy::asio::buffer_view(
            header_.c_str(), header_.size());

        lcy::asio::async_write(*stream_socket_, view, std::bind(
            &async_write_state::on_header_write_func, self,
                std::placeholders::_1, std::placeholders::_2));
    }

private:
    void on_header_write_func(lcy::system::error_code ec,
        std::size_t bytes_transferred)
    {
        if ( ec ) {
            func_(ec, bytes_transferred);
            return;
        }

        header_bytes_transferred_ = bytes_transferred;

        auto& buffer = message_->body_container();
        auto view = lcy::asio::buffer_view(
            static_cast<const char*>(buffer.data()), buffer.size());

        auto self = this->shared_from_this();

        lcy::asio::async_write(*stream_socket_, view, std::bind(
            &async_write_state::on_body_write_func, self,
                std::placeholders::_1, std::placeholders::_2));
    }

    void on_body_write_func(lcy::system::error_code ec,
        std::size_t bytes_transferred)
    {
        func_(ec, header_bytes_transferred_ + bytes_transferred);
    }

private:
    StreamSocket* stream_socket_;
    Message* message_;
    std::string header_;
    Function func_;
    std::size_t header_bytes_transferred_;
};

}   // namespace detail

template <
    typename StreamSocket,
    typename Message
>
std::size_t write_header(StreamSocket& stream_socket,
    Message& message, lcy::system::error_code& ec)
{
    auto header = message.header_to_string();
    auto view = lcy::asio::buffer_view(header.c_str(), header.size());

    return lcy::asio::write(stream_socket, view, ec);
}

template <
    typename StreamSocket,
    typename Message
>
std::size_t write_body(StreamSocket& stream_socket,
    Message& message, lcy::system::error_code& ec)
{
    auto& buffer = message.body_container();
    auto view = lcy::asio::buffer_view(
        static_cast<const char*>(buffer.data()), buffer.size());

    return lcy::asio::write(stream_socket, view, ec);
}

template <
    typename StreamSocket,
    typename Message
>
std::size_t write(StreamSocket& stream_socket,
    Message& message, lcy::system::error_code& ec)
{
    std::size_t transferred_bytes = 0;

    transferred_bytes += write_header(stream_socket, message, ec);
    if ( ec ) {
        return transferred_bytes;
    }

    transferred_bytes += write_body(stream_socket, message, ec);

    return transferred_bytes;
}

template <
    typename StreamSocket,
    typename Message
>
std::size_t write_header(StreamSocket& stream_socket, Message& message)
{
    lcy::system::error_code ec;
    std::size_t n = write_header(stream_socket, message, ec);
    if ( ec ) {
        detail::throw_http_exception(ec, "lcy::bytra::http::write_header");
    }

    return n;
}

template <
    typename StreamSocket,
    typename Message
>
std::size_t write_body(StreamSocket& stream_socket, Message& message)
{
    lcy::system::error_code ec;
    std::size_t n = write_body(stream_socket, message, ec);
    if ( ec ) {
        detail::throw_http_exception(ec, "lcy::bytra::http::write_body");
    }

    return n;
}

template <
    typename StreamSocket,
    typename Message
>
std::size_t write(StreamSocket& stream_socket, Message& message)
{
    lcy::system::error_code ec;
    std::size_t n = write(stream_socket, message, ec);
    if ( ec ) {
        detail::throw_http_exception(ec, "lcy::bytra::http::write");
    }

    return n;
}

template <
    typename StreamSocket,
    typename Message,
    typename Function
>
void async_write_header(StreamSocket& stream_socket,
    Message& message, Function func)
{
    std::make_shared<
        detail::async_write_header_state<Function>
    >(message.header_to_string(), std::move(func))->do_write(stream_socket);
}

template <
    typename StreamSocket,
    typename Message,
    typename Function
>
void async_write_body(StreamSocket& stream_socket,
    Message& message, Function func)
{
    auto& buffer = message.body_container();
    auto view = lcy::asio::buffer_view(
        static_cast<const char*>(buffer.data()), buffer.size());

    lcy::asio::async_write(stream_socket, view, std::move(func));
}

template <
    typename StreamSocket,
    typename Message,
    typename Function
>
void async_write(StreamSocket& stream_socket,
    Message& message, Function func)
{
    std::make_shared<detail::async_write_state<
        StreamSocket,
        Message,
        Function
    > >(stream_socket, message, std::move(func))->do_write();
}

}   // namespace http
}   // namespace bytra
}   // namespace lcy

#endif  // __LCY_BYTRA_HTTP_STREAM_WRITE_HPP__