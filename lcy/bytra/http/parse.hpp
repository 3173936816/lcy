#ifndef __LCY_BYTRA_HTTP_PARSE_HPP__
#define __LCY_BYTRA_HTTP_PARSE_HPP__

#include <memory>

#include "lcy/system/error.hpp"
#include "lcy/bytra/flat_buffer.hpp"
#include "lcy/bytra/http/detail/stream_parser.hpp"

namespace lcy
{
namespace bytra
{
namespace http
{

namespace detail
{

inline lcy::system::error_code make_buffer_full_error()
{
    return lcy::system::error_code(
        static_cast<int>(lcy::bytra::http::error::body_total_limit),
        lcy::system::generic_category());
}

template <
    typename StreamSocket,
    typename Message,
    typename Function
>
void parse_first_line(StreamSocket& stream_socket, Message& message, Function func)
{
    lcy::system::error_code io_ec;
    lcy::bytra::flat_buffer buffer;
    lcy::bytra::http::detail::stream_parser parser;
    std::size_t total_bytes_transferred = 0;

    for ( ;; ) {
        stream_parser::parse_result result = stream_parser::parse_result::wait_more;

        std::size_t consumed = parser.parse_first_line(
            static_cast<const char*>(buffer.data()), buffer.size(),
            [&](lcy::system::error_code ec,
                const stream_parser::first_line_view&,
                stream_parser::parse_result r) {
                io_ec  = ec;
                result = r;
            });

        buffer.consume(consumed);
        total_bytes_transferred += consumed;

        if ( result == stream_parser::parse_result::success ||
             result == stream_parser::parse_result::error_occurred ) {
            func(io_ec, total_bytes_transferred);
            return;
        }

        auto view = buffer.prepare(4096);
        if ( view.size() == 0 ) {
            func(make_buffer_full_error(), total_bytes_transferred);
            return;
        }

        std::size_t n = stream_socket.read_some(view, io_ec);

        if ( io_ec ) {
            func(io_ec, total_bytes_transferred);
            return;
        }

        buffer.commit(n);
    }
}

template <
    typename StreamSocket,
    typename Message,
    typename Function
>
class async_parse_first_line_state :
    public std::enable_shared_from_this<
        async_parse_first_line_state<
            StreamSocket,
            Message,
            Function
        >
    >
{
public:
    async_parse_first_line_state(StreamSocket& stream_socket,
                                 Message& message,
                                 Function func) :
        stream_socket_(&stream_socket),
        message_(&message),
        func_(std::move(func)),
        buffer_(),
        parser_(),
        total_bytes_transferred_(0),
        done_(false) {}

    void do_parse()
    {
        while ( !done_ ) {
            lcy::system::error_code ec;
            stream_parser::parse_result result = stream_parser::parse_result::wait_more;

            std::size_t consumed = parser_.parse_first_line(
                static_cast<const char*>(buffer_.data()), buffer_.size(),
                [&](lcy::system::error_code e,
                    const stream_parser::first_line_view&,
                    stream_parser::parse_result r) {
                    ec = e;
                    result = r;
                });

            buffer_.consume(consumed);
            total_bytes_transferred_ += consumed;

            if ( result == stream_parser::parse_result::success ||
                 result == stream_parser::parse_result::error_occurred ) {
                done_ = true;
                func_(ec, total_bytes_transferred_);
                return;
            }

            do_read();
            return;
        }
    }

    void do_read()
    {
        auto self = this->shared_from_this();
        auto view = buffer_.prepare(4096);

        if ( view.size() == 0 ) {
            done_ = true;
            func_(make_buffer_full_error(), total_bytes_transferred_);
            return;
        }

        stream_socket_->async_read_some(view,
                [this, self](const lcy::system::error_code& ec, std::size_t bytes_transferred) {
            if ( ec ) {
                done_ = true;
                func_(ec, total_bytes_transferred_);
                return;
            }

            buffer_.commit(bytes_transferred);
            do_parse();
        });
    }

private:
    StreamSocket* stream_socket_;
    Message* message_;
    Function func_;
    flat_buffer buffer_;
    stream_parser parser_;
    std::size_t total_bytes_transferred_;
    bool done_;
};

template <
    typename StreamSocket,
    typename Message,
    typename Function
>
void parse_fields(StreamSocket& stream_socket, Message& message, Function func)
{
    lcy::system::error_code io_ec;
    lcy::bytra::flat_buffer buffer;
    lcy::bytra::http::detail::stream_parser parser;
    std::size_t total_bytes_transferred = 0;

    for ( ;; ) {
        stream_parser::parse_result result = stream_parser::parse_result::wait_more;

        std::size_t consumed = parser.parse_field_line(
            static_cast<const char*>(buffer.data()), buffer.size(),
            [&](lcy::system::error_code ec,
                const stream_parser::field_line_view&,
                stream_parser::parse_result r) {
                io_ec  = ec;
                result = r;
            });

        buffer.consume(consumed);
        total_bytes_transferred += consumed;

        if ( result == stream_parser::parse_result::error_occurred ) {
            func(io_ec, total_bytes_transferred);
            return;
        }

        if ( result == stream_parser::parse_result::success ) {
            if ( parser.get_parse_phase() != stream_parser::parse_phase::field_line ) {
                func(io_ec, total_bytes_transferred);
                return;
            }

            continue;
        }

        auto view = buffer.prepare(4096);
        if ( view.size() == 0 ) {
            func(make_buffer_full_error(), total_bytes_transferred);
            return;
        }

        std::size_t n = stream_socket.read_some(view, io_ec);

        if ( io_ec ) {
            func(io_ec, total_bytes_transferred);
            return;
        }

        buffer.commit(n);
    }
}

template <
    typename StreamSocket,
    typename Message,
    typename Function
>
class async_parse_fields_state :
    public std::enable_shared_from_this<
        async_parse_fields_state<
            StreamSocket,
            Message,
            Function
        >
    >
{
public:
    async_parse_fields_state(StreamSocket& stream_socket,
                             Message& message,
                             Function func) :
        stream_socket_(&stream_socket),
        message_(&message),
        func_(std::move(func)),
        buffer_(),
        parser_(),
        total_bytes_transferred_(0),
        done_(false) {}

    void do_parse()
    {
        while ( !done_ ) {
            lcy::system::error_code ec;
            stream_parser::parse_result result = stream_parser::parse_result::wait_more;

            std::size_t consumed = parser_.parse_field_line(
                static_cast<const char*>(buffer_.data()), buffer_.size(),
                [&](lcy::system::error_code e,
                    const stream_parser::field_line_view&,
                    stream_parser::parse_result r) {
                    ec = e;
                    result = r;
                });

            buffer_.consume(consumed);
            total_bytes_transferred_ += consumed;

            if ( result == stream_parser::parse_result::error_occurred ) {
                done_ = true;
                func_(ec, total_bytes_transferred_);
                return;
            }

            if ( result == stream_parser::parse_result::success ) {
                if ( parser_.get_parse_phase() != stream_parser::parse_phase::field_line ) {
                    done_ = true;
                    func_(ec, total_bytes_transferred_);
                    return;
                }

                continue;
            }

            do_read();
            return;
        }
    }

    void do_read()
    {
        auto self = this->shared_from_this();
        auto view = buffer_.prepare(4096);

        if ( view.size() == 0 ) {
            done_ = true;
            func_(make_buffer_full_error(), total_bytes_transferred_);
            return;
        }

        stream_socket_->async_read_some(view,
                [this, self](const lcy::system::error_code& ec, std::size_t bytes_transferred) {
            if ( ec ) {
                done_ = true;
                func_(ec, total_bytes_transferred_);
                return;
            }

            buffer_.commit(bytes_transferred);
            do_parse();
        });
    }

private:
    StreamSocket* stream_socket_;
    Message* message_;
    Function func_;
    flat_buffer buffer_;
    stream_parser parser_;
    std::size_t total_bytes_transferred_;
    bool done_;
};

template <
    typename StreamSocket,
    typename Message,
    typename Function
>
void parse_header(StreamSocket& stream_socket, Message& message, Function func)
{
    lcy::system::error_code io_ec;
    lcy::bytra::flat_buffer buffer;
    lcy::bytra::http::detail::stream_parser parser;
    std::size_t total_bytes_transferred = 0;
    bool first_line_done = false;

    for ( ;; ) {
        if ( !first_line_done ) {
            stream_parser::parse_result result = stream_parser::parse_result::wait_more;

            std::size_t consumed = parser.parse_first_line(
                static_cast<const char*>(buffer.data()), buffer.size(),
                [&](lcy::system::error_code ec,
                    const stream_parser::first_line_view&,
                    stream_parser::parse_result r) {
                    io_ec  = ec;
                    result = r;
                });

            buffer.consume(consumed);
            total_bytes_transferred += consumed;

            if ( result == stream_parser::parse_result::error_occurred ) {
                func(io_ec, total_bytes_transferred);
                return;
            }

            if ( result == stream_parser::parse_result::success ) {
                first_line_done = true;
            } else {
                auto view = buffer.prepare(4096);
                if ( view.size() == 0 ) {
                    func(make_buffer_full_error(), total_bytes_transferred);
                    return;
                }

                std::size_t n = stream_socket.read_some(view, io_ec);

                if ( io_ec ) {
                    func(io_ec, total_bytes_transferred);
                    return;
                }

                buffer.commit(n);
                continue;
            }
        }

        stream_parser::parse_result result = stream_parser::parse_result::wait_more;

        std::size_t consumed = parser.parse_field_line(
            static_cast<const char*>(buffer.data()), buffer.size(),
            [&](lcy::system::error_code ec,
                const stream_parser::field_line_view&,
                stream_parser::parse_result r) {
                io_ec  = ec;
                result = r;
            });

        buffer.consume(consumed);
        total_bytes_transferred += consumed;

        if ( result == stream_parser::parse_result::error_occurred ) {
            func(io_ec, total_bytes_transferred);
            return;
        }

        if ( result == stream_parser::parse_result::success ) {
            if ( parser.get_parse_phase() != stream_parser::parse_phase::field_line ) {
                func(io_ec, total_bytes_transferred);
                return;
            }

            continue;
        }

        auto view = buffer.prepare(4096);
        if ( view.size() == 0 ) {
            func(make_buffer_full_error(), total_bytes_transferred);
            return;
        }

        std::size_t n = stream_socket.read_some(view, io_ec);

        if ( io_ec ) {
            func(io_ec, total_bytes_transferred);
            return;
        }

        buffer.commit(n);
    }
}

template <
    typename StreamSocket,
    typename Message,
    typename Function
>
class async_parse_header_state :
    public std::enable_shared_from_this<
        async_parse_header_state<
            StreamSocket,
            Message,
            Function
        >
    >
{
public:
    async_parse_header_state(StreamSocket& stream_socket,
                             Message& message,
                             Function func) :
        stream_socket_(&stream_socket),
        message_(&message),
        func_(std::move(func)),
        buffer_(),
        parser_(),
        total_bytes_transferred_(0),
        first_line_done_(false),
        done_(false) {}

    void do_parse()
    {
        while ( !done_ ) {
            if ( !first_line_done_ ) {
                lcy::system::error_code ec;
                stream_parser::parse_result result = stream_parser::parse_result::wait_more;

                std::size_t consumed = parser_.parse_first_line(
                    static_cast<const char*>(buffer_.data()), buffer_.size(),
                    [&](lcy::system::error_code e,
                        const stream_parser::first_line_view&,
                        stream_parser::parse_result r) {
                        ec = e;
                        result = r;
                    });

                buffer_.consume(consumed);
                total_bytes_transferred_ += consumed;

                if ( result == stream_parser::parse_result::error_occurred ) {
                    done_ = true;
                    func_(ec, total_bytes_transferred_);
                    return;
                }

                if ( result == stream_parser::parse_result::success ) {
                    first_line_done_ = true;
                } else {
                    do_read();
                    return;
                }
            }

            lcy::system::error_code ec;
            stream_parser::parse_result result = stream_parser::parse_result::wait_more;

            std::size_t consumed = parser_.parse_field_line(
                static_cast<const char*>(buffer_.data()), buffer_.size(),
                [&](lcy::system::error_code e,
                    const stream_parser::field_line_view&,
                    stream_parser::parse_result r) {
                    ec = e;
                    result = r;
                });

            buffer_.consume(consumed);
            total_bytes_transferred_ += consumed;

            if ( result == stream_parser::parse_result::error_occurred ) {
                done_ = true;
                func_(ec, total_bytes_transferred_);
                return;
            }

            if ( result == stream_parser::parse_result::success ) {
                if ( parser_.get_parse_phase() != stream_parser::parse_phase::field_line ) {
                    done_ = true;
                    func_(ec, total_bytes_transferred_);
                    return;
                }

                continue;
            }

            do_read();
            return;
        }
    }

    void do_read()
    {
        auto self = this->shared_from_this();
        auto view = buffer_.prepare(4096);

        if ( view.size() == 0 ) {
            done_ = true;
            func_(make_buffer_full_error(), total_bytes_transferred_);
            return;
        }

        stream_socket_->async_read_some(view,
                [this, self](const lcy::system::error_code& ec, std::size_t bytes_transferred) {
            if ( ec ) {
                done_ = true;
                func_(ec, total_bytes_transferred_);
                return;
            }

            buffer_.commit(bytes_transferred);
            do_parse();
        });
    }

private:
    StreamSocket* stream_socket_;
    Message* message_;
    Function func_;
    flat_buffer buffer_;
    stream_parser parser_;
    std::size_t total_bytes_transferred_;
    bool first_line_done_;
    bool done_;
};

template <
    typename StreamSocket,
    typename Message,
    typename Function
>
void parse_body(StreamSocket& stream_socket, Message& message, Function func)
{
    lcy::system::error_code io_ec;
    lcy::bytra::flat_buffer buffer;
    lcy::bytra::http::detail::stream_parser parser;
    std::size_t total_bytes_transferred = 0;

    for ( ;; ) {
        stream_parser::parse_result result = stream_parser::parse_result::wait_more;
        std::size_t consumed = 0;

        if ( parser.is_chunk_mode() ) {
            consumed = parser.parse_chunk(
                static_cast<const char*>(buffer.data()), buffer.size(),
                [&](lcy::system::error_code ec,
                    const stream_parser::chunk_view&,
                    stream_parser::parse_result r) {
                    io_ec  = ec;
                    result = r;
                });
        } else {
            consumed = parser.parse_body(
                static_cast<const char*>(buffer.data()), buffer.size(),
                [&](lcy::system::error_code ec,
                    const stream_parser::body_view&,
                    stream_parser::parse_result r) {
                    io_ec  = ec;
                    result = r;
                });
        }

        buffer.consume(consumed);
        total_bytes_transferred += consumed;

        if ( result == stream_parser::parse_result::error_occurred ) {
            func(io_ec, total_bytes_transferred);
            return;
        }

        if ( parser.get_parse_phase() == stream_parser::parse_phase::complete ) {
            func(io_ec, total_bytes_transferred);
            return;
        }

        if ( result == stream_parser::parse_result::success && consumed > 0 ) {
            continue;
        }

        auto view = buffer.prepare(4096);
        if ( view.size() == 0 ) {
            func(make_buffer_full_error(), total_bytes_transferred);
            return;
        }

        std::size_t n = stream_socket.read_some(view, io_ec);

        if ( io_ec ) {
            func(io_ec, total_bytes_transferred);
            return;
        }

        buffer.commit(n);
    }
}

template <
    typename StreamSocket,
    typename Message,
    typename Function
>
class async_parse_body_state :
    public std::enable_shared_from_this<
        async_parse_body_state<
            StreamSocket,
            Message,
            Function
        >
    >
{
public:
    async_parse_body_state(StreamSocket& stream_socket,
                           Message& message,
                           Function func) :
        stream_socket_(&stream_socket),
        message_(&message),
        func_(std::move(func)),
        buffer_(),
        parser_(),
        total_bytes_transferred_(0),
        done_(false) {}

    void do_parse()
    {
        while ( !done_ ) {
            lcy::system::error_code ec;
            stream_parser::parse_result result = stream_parser::parse_result::wait_more;
            std::size_t consumed = 0;

            if ( parser_.is_chunk_mode() ) {
                consumed = parser_.parse_chunk(
                    static_cast<const char*>(buffer_.data()), buffer_.size(),
                    [&](lcy::system::error_code e,
                        const stream_parser::chunk_view&,
                        stream_parser::parse_result r) {
                        ec = e;
                        result = r;
                    });
            } else {
                consumed = parser_.parse_body(
                    static_cast<const char*>(buffer_.data()), buffer_.size(),
                    [&](lcy::system::error_code e,
                        const stream_parser::body_view&,
                        stream_parser::parse_result r) {
                        ec = e;
                        result = r;
                    });
            }

            buffer_.consume(consumed);
            total_bytes_transferred_ += consumed;

            if ( result == stream_parser::parse_result::error_occurred ) {
                done_ = true;
                func_(ec, total_bytes_transferred_);
                return;
            }

            if ( parser_.get_parse_phase() == stream_parser::parse_phase::complete ) {
                done_ = true;
                func_(ec, total_bytes_transferred_);
                return;
            }

            if ( result == stream_parser::parse_result::success && consumed > 0 ) {
                continue;
            }

            do_read();
            return;
        }
    }

    void do_read()
    {
        auto self = this->shared_from_this();
        auto view = buffer_.prepare(4096);

        if ( view.size() == 0 ) {
            done_ = true;
            func_(make_buffer_full_error(), total_bytes_transferred_);
            return;
        }

        stream_socket_->async_read_some(view,
                [this, self](const lcy::system::error_code& ec, std::size_t bytes_transferred) {
            if ( ec ) {
                done_ = true;
                func_(ec, total_bytes_transferred_);
                return;
            }

            buffer_.commit(bytes_transferred);
            do_parse();
        });
    }

private:
    StreamSocket* stream_socket_;
    Message* message_;
    Function func_;
    flat_buffer buffer_;
    stream_parser parser_;
    std::size_t total_bytes_transferred_;
    bool done_;
};

template <
    typename StreamSocket,
    typename Message,
    typename Function
>
void parse(StreamSocket& stream_socket, Message& message, Function func)
{
    lcy::system::error_code io_ec;
    lcy::bytra::flat_buffer buffer;
    lcy::bytra::http::detail::stream_parser parser;
    std::size_t total_bytes_transferred = 0;

    for ( ;; ) {
        stream_parser::parse_result result = stream_parser::parse_result::wait_more;
        std::size_t consumed = 0;

        if ( parser.get_parse_phase() == stream_parser::parse_phase::first_line ) {
            consumed = parser.parse_first_line(
                static_cast<const char*>(buffer.data()), buffer.size(),
                [&](lcy::system::error_code ec,
                    const stream_parser::first_line_view&,
                    stream_parser::parse_result r) {
                    io_ec  = ec;
                    result = r;
                });
        } else if ( parser.get_parse_phase() == stream_parser::parse_phase::field_line ) {
            consumed = parser.parse_field_line(
                static_cast<const char*>(buffer.data()), buffer.size(),
                [&](lcy::system::error_code ec,
                    const stream_parser::field_line_view&,
                    stream_parser::parse_result r) {
                    io_ec  = ec;
                    result = r;
                });
        } else if ( parser.get_parse_phase() == stream_parser::parse_phase::body ) {
            if ( parser.is_chunk_mode() ) {
                consumed = parser.parse_chunk(
                    static_cast<const char*>(buffer.data()), buffer.size(),
                    [&](lcy::system::error_code ec,
                        const stream_parser::chunk_view&,
                        stream_parser::parse_result r) {
                        io_ec  = ec;
                        result = r;
                    });
            } else {
                consumed = parser.parse_body(
                    static_cast<const char*>(buffer.data()), buffer.size(),
                    [&](lcy::system::error_code ec,
                        const stream_parser::body_view&,
                        stream_parser::parse_result r) {
                        io_ec  = ec;
                        result = r;
                    });
            }
        } else if ( parser.get_parse_phase() == stream_parser::parse_phase::trailer ) {
            consumed = parser.parse_trailer(
                static_cast<const char*>(buffer.data()), buffer.size(),
                [&](lcy::system::error_code ec,
                    const stream_parser::field_line_view&,
                    stream_parser::parse_result r) {
                    io_ec  = ec;
                    result = r;
                });
        }

        buffer.consume(consumed);
        total_bytes_transferred += consumed;

        if ( result == stream_parser::parse_result::error_occurred ) {
            func(io_ec, total_bytes_transferred);
            return;
        }

        if ( parser.get_parse_phase() == stream_parser::parse_phase::complete ) {
            func(io_ec, total_bytes_transferred);
            return;
        }

        if ( result == stream_parser::parse_result::success && consumed > 0 ) {
            continue;
        }

        auto view = buffer.prepare(4096);
        if ( view.size() == 0 ) {
            func(make_buffer_full_error(), total_bytes_transferred);
            return;
        }

        std::size_t n = stream_socket.read_some(view, io_ec);

        if ( io_ec ) {
            func(io_ec, total_bytes_transferred);
            return;
        }

        buffer.commit(n);
    }
}

template <
    typename StreamSocket,
    typename Message,
    typename Function
>
class async_parse_state :
    public std::enable_shared_from_this<
        async_parse_state<
            StreamSocket,
            Message,
            Function
        >
    >
{
public:
    async_parse_state(StreamSocket& stream_socket,
                      Message& message,
                      Function func) :
        stream_socket_(&stream_socket),
        message_(&message),
        func_(std::move(func)),
        buffer_(),
        parser_(),
        total_bytes_transferred_(0),
        done_(false) {}

    void do_parse()
    {
        while ( !done_ ) {
            lcy::system::error_code ec;
            stream_parser::parse_result result = stream_parser::parse_result::wait_more;
            std::size_t consumed = 0;

            if ( parser_.get_parse_phase() == stream_parser::parse_phase::first_line ) {
                consumed = parser_.parse_first_line(
                    static_cast<const char*>(buffer_.data()), buffer_.size(),
                    [&](lcy::system::error_code e,
                        const stream_parser::first_line_view&,
                        stream_parser::parse_result r) {
                        ec = e;
                        result = r;
                    });
            } else if ( parser_.get_parse_phase() == stream_parser::parse_phase::field_line ) {
                consumed = parser_.parse_field_line(
                    static_cast<const char*>(buffer_.data()), buffer_.size(),
                    [&](lcy::system::error_code e,
                        const stream_parser::field_line_view&,
                        stream_parser::parse_result r) {
                        ec = e;
                        result = r;
                    });
            } else if ( parser_.get_parse_phase() == stream_parser::parse_phase::body ) {
                if ( parser_.is_chunk_mode() ) {
                    consumed = parser_.parse_chunk(
                        static_cast<const char*>(buffer_.data()), buffer_.size(),
                        [&](lcy::system::error_code e,
                            const stream_parser::chunk_view&,
                            stream_parser::parse_result r) {
                            ec = e;
                            result = r;
                        });
                } else {
                    consumed = parser_.parse_body(
                        static_cast<const char*>(buffer_.data()), buffer_.size(),
                        [&](lcy::system::error_code e,
                            const stream_parser::body_view&,
                            stream_parser::parse_result r) {
                            ec = e;
                            result = r;
                        });
                }
            } else if ( parser_.get_parse_phase() == stream_parser::parse_phase::trailer ) {
                consumed = parser_.parse_trailer(
                    static_cast<const char*>(buffer_.data()), buffer_.size(),
                    [&](lcy::system::error_code e,
                        const stream_parser::field_line_view&,
                        stream_parser::parse_result r) {
                        ec = e;
                        result = r;
                    });
            }

            buffer_.consume(consumed);
            total_bytes_transferred_ += consumed;

            if ( result == stream_parser::parse_result::error_occurred ) {
                done_ = true;
                func_(ec, total_bytes_transferred_);
                return;
            }

            if ( parser_.get_parse_phase() == stream_parser::parse_phase::complete ) {
                done_ = true;
                func_(ec, total_bytes_transferred_);
                return;
            }

            if ( result == stream_parser::parse_result::success && consumed > 0 ) {
                continue;
            }

            do_read();
            return;
        }
    }

    void do_read()
    {
        auto self = this->shared_from_this();
        auto view = buffer_.prepare(4096);

        if ( view.size() == 0 ) {
            done_ = true;
            func_(make_buffer_full_error(), total_bytes_transferred_);
            return;
        }

        stream_socket_->async_read_some(view,
                [this, self](const lcy::system::error_code& ec, std::size_t bytes_transferred) {
            if ( ec ) {
                done_ = true;
                func_(ec, total_bytes_transferred_);
                return;
            }

            buffer_.commit(bytes_transferred);
            do_parse();
        });
    }

private:
    StreamSocket* stream_socket_;
    Message* message_;
    Function func_;
    flat_buffer buffer_;
    stream_parser parser_;
    std::size_t total_bytes_transferred_;
    bool done_;
};

}   // namespace detail

template <
    typename StreamSocket,
    typename Message,
    typename Function
>
void parse_first_line(StreamSocket& stream_socket, Message& message, Function func)
{
    detail::parse_first_line(stream_socket, message, std::move(func));
}

template <
    typename StreamSocket,
    typename Message,
    typename Function
>
void parse_fields(StreamSocket& stream_socket, Message& message, Function func)
{
    detail::parse_fields(stream_socket, message, std::move(func));
}

template <
    typename StreamSocket,
    typename Message,
    typename Function
>
void parse_header(StreamSocket& stream_socket, Message& message, Function func)
{
    detail::parse_header(stream_socket, message, std::move(func));
}

template <
    typename StreamSocket,
    typename Message,
    typename Function
>
void parse_body(StreamSocket& stream_socket, Message& message, Function func)
{
    detail::parse_body(stream_socket, message, std::move(func));
}

template <
    typename StreamSocket,
    typename Message,
    typename Function
>
void parse(StreamSocket& stream_socket, Message& message, Function func)
{
    detail::parse(stream_socket, message, std::move(func));
}

template <
    typename StreamSocket,
    typename Message,
    typename Function
>
void async_parse_first_line(StreamSocket& stream_socket, Message& message, Function func)
{
    std::make_shared<detail::async_parse_first_line_state<
        StreamSocket,
        Message,
        Function
    > >(stream_socket, message, std::move(func))->do_parse();
}

template <
    typename StreamSocket,
    typename Message,
    typename Function
>
void async_parse_fields(StreamSocket& stream_socket, Message& message, Function func)
{
    std::make_shared<detail::async_parse_fields_state<
        StreamSocket,
        Message,
        Function
    > >(stream_socket, message, std::move(func))->do_parse();
}

template <
    typename StreamSocket,
    typename Message,
    typename Function
>
void async_parse_header(StreamSocket& stream_socket, Message& message, Function func)
{
    std::make_shared<detail::async_parse_header_state<
        StreamSocket,
        Message,
        Function
    > >(stream_socket, message, std::move(func))->do_parse();
}

template <
    typename StreamSocket,
    typename Message,
    typename Function
>
void async_parse_body(StreamSocket& stream_socket, Message& message, Function func)
{
    std::make_shared<detail::async_parse_body_state<
        StreamSocket,
        Message,
        Function
    > >(stream_socket, message, std::move(func))->do_parse();
}

template <
    typename StreamSocket,
    typename Message,
    typename Function
>
void async_parse(StreamSocket& stream_socket, Message& message, Function func)
{
    std::make_shared<detail::async_parse_state<
        StreamSocket,
        Message,
        Function
    > >(stream_socket, message, std::move(func))->do_parse();
}

}   // namespace http
}   // namespace bytra
}   // namespace lcy

#endif  // __LCY_BYTRA_HTTP_PARSE_HPP__