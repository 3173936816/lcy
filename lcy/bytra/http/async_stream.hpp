#ifndef __LCY_BYTRA_HTTP_ASYNC_STREAM_HPP__
#define __LCY_BYTRA_HTTP_ASYNC_STREAM_HPP__

#include <cstddef>
#include <memory>
#include <utility>

#include "lcy/system/error.hpp"
#include "lcy/asio/async_stream.hpp"
#include "lcy/bytra/http/error.hpp"
#include "lcy/bytra/flat_buffer.hpp"
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
    typename FlatBuffer,
    typename HttpParser,
    typename Message,
    typename Function
>
class async_parse_state :
    public std::enable_shared_from_this<
        async_parse_state<
            StreamSocket,
            FlatBuffer,
            HttpParser,
            Message,
            Function
        >
    >
{
public:
    enum mode { header, body, full };

    async_parse_state(StreamSocket& stream_socket,
                      FlatBuffer& buffer,
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
        start_parsed_(parser.get_parsed_bytes()) {}

    void do_parse()
    {
        auto self = this->shared_from_this();

        auto view = buffer_->prepare(4096);
        stream_socket_->async_read_some(view,
            [this, self](lcy::system::error_code ec, std::size_t bytes_transferred) {
                if ( ec ) {
                    func_(ec, parser_->get_parsed_bytes());
                    return;
                }

                buffer_->commit(bytes_transferred);

                lcy::system::error_code pec;
                const char* data = static_cast<const char*>(buffer_->data());
                const std::size_t size = buffer_->size();

                std::size_t consumed = 0;
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

                if ( pec ) {
                    if ( pec == lcy::bytra::http::error::wait_more ) {
                        do_parse();
                        return;
                    }
                    func_(pec, parser_->get_parsed_bytes());
                    return;
                }

                func_(pec, parser_->get_parsed_bytes());
            });
    }

private:
    StreamSocket* stream_socket_;
    FlatBuffer* buffer_;
    HttpParser* parser_;
    Message* message_;
    Function func_;
    mode mode_;
    std::size_t start_parsed_;
};

}   // namespace detail

template <
    typename StreamSocket,
    typename FlatBuffer,
    typename HttpParser,
    typename Message,
    typename Function
>
void async_read_header(StreamSocket& stream_socket,
    FlatBuffer& buffer, HttpParser& parser, Message& message, Function func)
{
    std::make_shared<detail::async_parse_state<
        StreamSocket,
        FlatBuffer,
        HttpParser,
        Message,
        Function
    > >(stream_socket, buffer, parser, message,
        std::move(func),
        detail::async_parse_state<
            StreamSocket,
            FlatBuffer,
            HttpParser,
            Message,
            Function
        >::mode::header)->do_parse();
}

template <
    typename StreamSocket,
    typename FlatBuffer,
    typename HttpParser,
    typename Message,
    typename Function
>
void async_read_body(StreamSocket& stream_socket,
    FlatBuffer& buffer, HttpParser& parser, Message& message, Function func)
{
    std::make_shared<detail::async_parse_state<
        StreamSocket,
        FlatBuffer,
        HttpParser,
        Message,
        Function
    > >(stream_socket, buffer, parser, message,
        std::move(func),
        detail::async_parse_state<
            StreamSocket,
            FlatBuffer,
            HttpParser,
            Message,
            Function
        >::mode::body)->do_parse();
}

template <
    typename StreamSocket,
    typename FlatBuffer,
    typename HttpParser,
    typename Message,
    typename Function
>
void async_read(StreamSocket& stream_socket,
    FlatBuffer& buffer, HttpParser& parser, Message& message, Function func)
{
    std::make_shared<detail::async_parse_state<
        StreamSocket,
        FlatBuffer,
        HttpParser,
        Message,
        Function
    > >(stream_socket, buffer, parser, message,
        std::move(func),
        detail::async_parse_state<
            StreamSocket,
            FlatBuffer,
            HttpParser,
            Message,
            Function
        >::mode::full)->do_parse();
}

template <
    typename StreamSocket,
    typename HttpParser,
    typename Message,
    typename Function
>
void async_read(StreamSocket& stream_socket,
    HttpParser& parser, Message& message, Function func)
{
    auto buffer = std::make_shared<flat_buffer>();
    auto func_wrapper = [buffer, func](
            lcy::system::error_code ec, std::size_t parsed_bytes) {
        func(ec, parsed_bytes);
    };

    std::make_shared<detail::async_parse_state<
        StreamSocket,
        flat_buffer,
        HttpParser,
        Message,
        decltype(func_wrapper)
    > >(stream_socket, *buffer, parser, message,
        std::move(func_wrapper),
        detail::async_parse_state<
            StreamSocket,
            flat_buffer,
            HttpParser,
            Message,
            decltype(func_wrapper)
        >::mode::full)->do_parse();
}


template <
    typename StreamSocket,
    typename Message,
    typename Function
>
void async_write(StreamSocket& stream_socket, Message& message, Function func)
{
    auto message_str = std::make_shared<std::string>(message.header_to_string() +
        std::string(static_cast<char*>(message.body_container().data()), message.body_container().size())); 
    auto func_wrapper = [message_str, func](lcy::system::error_code ec, std::size_t n){
        func(ec, n);
    };
    lcy::asio::async_write(stream_socket, lcy::asio::buffer_view(message_str->c_str(), message_str->size()), std::move(func_wrapper));
}

}   // namespace http
}   // namespace bytra
}   // namespace lcy

#endif  // __LCY_BYTRA_HTTP_ASYNC_STREAM_HPP__