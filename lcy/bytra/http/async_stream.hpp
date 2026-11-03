#ifndef __LCY_BYTRA_HTTP_ASYNC_STREAM_HPP__
#define __LCY_BYTRA_HTTP_ASYNC_STREAM_HPP__

#include <cstddef>
#include <memory>
#include <utility>

#include "lcy/system/error.hpp"
#include "lcy/asio/async_stream.hpp"
#include "lcy/bytra/flat_buffer.hpp"
#include "lcy/bytra/http/error.hpp"
#include "lcy/bytra/http/request_parser.hpp"
#include "lcy/bytra/http/response_parser.hpp"

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
        parse_bytes_(0) {}

    void do_parse()
    {
        auto self = this->shared_from_this();

        auto view = buffer_->prepare(4096);
        stream_socket_->async_read_some(view,
                [this, self](lcy::system::error_code ec, std::size_t bytes_transferred) {
            if ( ec ) {
                func_(ec, parse_bytes_);
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
            parse_bytes_ += consumed;

            if ( pec ) {
                if ( pec == lcy::bytra::http::error::wait_more ) {
                    do_parse();
                    return;
                }
                func_(pec, parse_bytes_);
                return;
            }

            func_(pec, parse_bytes_);
        });
    }

private:
    StreamSocket* stream_socket_;
    DynamicBuffer* buffer_;
    HttpParser* parser_;
    Message* message_;
    Function func_;
    mode mode_;
    std::size_t parse_bytes_;
};

}   // namespace detail

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

template <
    typename StreamSocket,
    typename Message,
    typename Function
>
void async_write_header(StreamSocket& stream_socket, Message& message, Function func)
{
    auto header_sptr = std::make_shared<std::string>(message.header_to_string());
    auto buffer_view = lcy::asio::buffer_view(header_sptr->c_str(), header_sptr->size());
    auto func_wrapper = [header_sptr, func](
            lcy::system::error_code ec, std::size_t bytes_transferred){
        func(ec, bytes_transferred);
    };

    lcy::asio::async_write(stream_socket, buffer_view, std::move(func_wrapper));
}

template <
    typename StreamSocket,
    typename Message,
    typename Function
>
void async_write_body(StreamSocket& stream_socket, Message& message, Function func)
{
    auto& buffer = message.body_container();
    auto buffer_view = lcy::asio::buffer_view(static_cast<const char*>(buffer.data()), buffer.size());

    lcy::asio::async_write(stream_socket, buffer_view, std::move(func));
}

template <
    typename StreamSocket,
    typename Message,
    typename Function
>
void async_write(StreamSocket& stream_socket, Message& message, Function func)
{
    auto func_wrapper = [&stream_socket, &message, func](
            lcy::system::error_code ec, std::size_t bytes_transferred){
        if ( ec ) {
            func(ec, bytes_transferred);
            return;
        }

        async_write_body(stream_socket, message, std::move(func));
    };

    async_write_header(stream_socket, message, std::move(func_wrapper));
}

}   // namespace http
}   // namespace bytra
}   // namespace lcy

#endif  // __LCY_BYTRA_HTTP_ASYNC_STREAM_HPP__