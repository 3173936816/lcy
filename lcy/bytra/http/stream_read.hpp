#ifndef __LCY_BYTRA_HTTP_STREAM_READ_HPP__
#define __LCY_BYTRA_HTTP_STREAM_READ_HPP__

#include <cstddef>
#include <memory>
#include <utility>

#include "lcy/system/error.hpp"
#include "lcy/asio/stream_read.hpp"
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

template <
    typename StreamSocket,
    typename DynamicBuffer,
    typename HttpParser,
    typename Message,
    typename Function
>
class async_parse_state :
    public std::enable_shared_from_this<
        async_parse_state<
            StreamSocket,
            DynamicBuffer,
            HttpParser,
            Message,
            Function
        >
    >
{
public:
    enum mode { header, body, full };

    async_parse_state(StreamSocket& stream_socket,
                      DynamicBuffer& buffer,
                      HttpParser& parser,
                      Message& message,
                      Function func,
                      mode m) :
        stream_socket_(&stream_socket),
        buffer_(&buffer),
        parser_(&parser),
        message_(&message),
        func_(std::move(func)),
        mode_(m),
        parsed_bytes_(0) {}

    void do_parse()
    {
        auto self = this->shared_from_this();
        auto view = buffer_->prepare(4096);
        auto on_read_func = [this, self](lcy::system::error_code ec,
            std::size_t bytes_transferred)
        {
            if ( ec ) {
                func_(ec, parsed_bytes_);
                return;
            }

            buffer_->commit(bytes_transferred);

            std::size_t consumed = 0;
            lcy::system::error_code pec;
            const char* data = static_cast<const char*>(buffer_->data());
            const std::size_t size = buffer_->size();

            if ( mode_ == mode::header ) {
                consumed = parser_->parse_header(data, size, *message_, pec);
            } else if ( mode_ == mode::body ) {
                consumed = parser_->parse_body(data, size, *message_, pec);
            } else {
                consumed = parser_->parse(data, size, *message_, pec);
            }

            if ( consumed > 0 ) {
                buffer_->consume(consumed);
            }
            parsed_bytes_ += consumed;

            if ( pec ) {
                if ( pec == lcy::bytra::http::error::wait_more ) {
                    do_parse();
                    return;
                }
                func_(pec, parsed_bytes_);
                return;
            }

            func_(pec, parsed_bytes_);
        };

        stream_socket_->async_read_some(view, std::move(on_read_func));
    }

private:
    StreamSocket* stream_socket_;
    DynamicBuffer* buffer_;
    HttpParser* parser_;
    Message* message_;
    Function func_;
    mode mode_;
    std::size_t parsed_bytes_;
};

}   // namespace detail

template <
    typename StreamSocket,
    typename DynamicBuffer,
    typename HttpParser,
    typename Message
>
std::size_t read_header(StreamSocket& stream_socket,
    DynamicBuffer& buffer, HttpParser& parser,
    Message& message, lcy::system::error_code& ec)
{
    std::size_t parsed_bytes = 0;

    for ( ;; ) {
        auto view = buffer.prepare(4096);
        std::size_t n = stream_socket.read_some(view, ec);
        if ( ec ) {
            break;
        }

        buffer.commit(n);

        n = parser.parse_header(buffer.data(), buffer.size(), message, ec);

        parsed_bytes += n;
        buffer.consume(n);

        if ( ec != lcy::bytra::http::error::wait_more ) {
            break;
        }
    }

    return parsed_bytes;
}

template <
    typename StreamSocket,
    typename DynamicBuffer,
    typename HttpParser,
    typename Message
>
std::size_t read_body(StreamSocket& stream_socket,
    DynamicBuffer& buffer, HttpParser& parser,
    Message& message, lcy::system::error_code& ec)
{
    std::size_t parsed_bytes = 0;

    for ( ;; ) {
        auto view = buffer.prepare(4096);
        std::size_t n = stream_socket.read_some(view, ec);
        if ( ec ) {
            break;
        }

        buffer.commit(n);

        n = parser.parse_body(buffer.data(), buffer.size(), message, ec);

        parsed_bytes += n;
        buffer.consume(n);

        if ( ec != lcy::bytra::http::error::wait_more ) {
            break;
        }
    }

    return parsed_bytes;
}

template <
    typename StreamSocket,
    typename DynamicBuffer,
    typename HttpParser,
    typename Message
>
std::size_t read(StreamSocket& stream_socket,
    DynamicBuffer& buffer, HttpParser& parser,
    Message& message, lcy::system::error_code& ec)
{
    std::size_t parsed_bytes = 0;

    for ( ;; ) {
        auto view = buffer.prepare(4096);
        std::size_t n = stream_socket.read_some(view, ec);
        if ( ec ) {
            break;
        }

        buffer.commit(n);

        n = parser.parse(buffer.data(), buffer.size(), message, ec);

        parsed_bytes += n;
        buffer.consume(n);

        if ( ec != lcy::bytra::http::error::wait_more ) {
            break;
        }
    }

    return parsed_bytes;
}

template <
    typename StreamSocket,
    typename DynamicBuffer,
    typename HttpParser,
    typename Message
>
std::size_t read_header(StreamSocket& stream_socket,
    DynamicBuffer& buffer, HttpParser& parser, Message& message)
{
    lcy::system::error_code ec;
    std::size_t n = read_header(stream_socket, buffer, parser, message, ec);
    if ( ec ) {
        detail::throw_http_exception(ec, "lcy::bytra::http::read_header");
    }

    return n;
}

template <
    typename StreamSocket,
    typename DynamicBuffer,
    typename HttpParser,
    typename Message
>
std::size_t read_body(StreamSocket& stream_socket,
    DynamicBuffer& buffer, HttpParser& parser, Message& message)
{
    lcy::system::error_code ec;
    std::size_t n = read_body(stream_socket, buffer, parser, message, ec);
    if ( ec ) {
        detail::throw_http_exception(ec, "lcy::bytra::http::read_body");
    }

     return n;
}

template <
    typename StreamSocket,
    typename DynamicBuffer,
    typename HttpParser,
    typename Message
>
std::size_t read(StreamSocket& stream_socket,
    DynamicBuffer& buffer, HttpParser& parser, Message& message)
{
    lcy::system::error_code ec;
    std::size_t n = read(stream_socket, buffer, parser, message, ec);
    if ( ec ) {
        detail::throw_http_exception(ec, "lcy::bytra::http::read");
    }

     return n;
}

template <
    typename StreamSocket,
    typename DynamicBuffer,
    typename HttpParser,
    typename Message,
    typename Function
>
void async_read_header(StreamSocket& stream_socket,
    DynamicBuffer& buffer, HttpParser& parser, Message& message, Function func)
{
    std::make_shared<detail::async_parse_state<
        StreamSocket,
        DynamicBuffer,
        HttpParser,
        Message,
        Function
    > >(stream_socket, buffer, parser, message,
        std::move(func),
        detail::async_parse_state<
            StreamSocket,
            DynamicBuffer,
            HttpParser,
            Message,
            Function
        >::mode::header)->do_parse();
}

template <
    typename StreamSocket,
    typename DynamicBuffer,
    typename HttpParser,
    typename Message,
    typename Function
>
void async_read_body(StreamSocket& stream_socket,
    DynamicBuffer& buffer, HttpParser& parser, Message& message, Function func)
{
    std::make_shared<detail::async_parse_state<
        StreamSocket,
        DynamicBuffer,
        HttpParser,
        Message,
        Function
    > >(stream_socket, buffer, parser, message,
        std::move(func),
        detail::async_parse_state<
            StreamSocket,
            DynamicBuffer,
            HttpParser,
            Message,
            Function
        >::mode::body)->do_parse();
}

template <
    typename StreamSocket,
    typename DynamicBuffer,
    typename HttpParser,
    typename Message,
    typename Function
>
void async_read(StreamSocket& stream_socket,
    DynamicBuffer& buffer, HttpParser& parser, Message& message, Function func)
{
    std::make_shared<detail::async_parse_state<
        StreamSocket,
        DynamicBuffer,
        HttpParser,
        Message,
        Function
    > >(stream_socket, buffer, parser, message,
        std::move(func),
        detail::async_parse_state<
            StreamSocket,
            DynamicBuffer,
            HttpParser,
            Message,
            Function
        >::mode::full)->do_parse();
}

}   // namespace http
}   // namespace bytra
}   // namespace lcy

#endif  // __LCY_BYTRA_HTTP_STREAM_READ_HPP__