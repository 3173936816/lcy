#ifndef __LCY_BYTRA_HTTP_STREAM_WRITE_HPP__
#define __LCY_BYTRA_HTTP_STREAM_WRITE_HPP__

#include <cstddef>
#include <memory>
#include <utility>

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
    auto header_sptr = std::make_shared<std::string>(message.header_to_string());
    auto view = lcy::asio::buffer_view(header_sptr->c_str(), header_sptr->size());
    auto on_write_func = [header_sptr, func](
        lcy::system::error_code ec, std::size_t bytes_transferred)
    {
        func(ec, bytes_transferred);
    };

    lcy::asio::async_write(stream_socket, view, std::move(on_write_func));
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
    auto on_header_write_func = [&stream_socket, &message, func](
        lcy::system::error_code header_ec, std::size_t header_bytes_transferred)
    {
        if ( header_ec ) {
            func(header_ec, header_bytes_transferred);
            return;
        }

        auto on_body_write_func = [func, header_bytes_transferred](
            lcy::system::error_code body_ec, std::size_t body_bytes_transferred)
        {
            func(body_ec, header_bytes_transferred + body_bytes_transferred);
        };

        async_write_body(stream_socket, message, std::move(on_body_write_func));
    };

    async_write_header(stream_socket, message, std::move(on_header_write_func));
}

}   // namespace http
}   // namespace bytra
}   // namespace lcy

#endif  // __LCY_BYTRA_HTTP_STREAM_WRITE_HPP__