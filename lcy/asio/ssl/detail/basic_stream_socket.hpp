#ifndef __LCY_ASIO_SSL_DETAIL_BASIC_STREAM_SOCKET_HPP__
#define __LCY_ASIO_SSL_DETAIL_BASIC_STREAM_SOCKET_HPP__

#include <cstddef>
#include <functional>
#include <openssl/ssl.h>
#include <openssl/bio.h>

#include "lcy/noncopyable.hpp"
#include "lcy/asio/ip/tcp.hpp"
#include "lcy/asio/buffer_view.hpp"
#include "lcy/asio/ssl/detail/error.hpp"
#include "lcy/asio/ssl/detail/throw_ssl_exception.hpp"
#include "lcy/asio/ssl/detail/stream_socket_core.hpp"

namespace lcy
{
namespace asio
{
namespace ssl
{
namespace detail
{

template <typename Context>
class basic_stream_socket :
    private noncopyable
{
public:
    typedef Context context_type;

    typedef stream_socket_core::handshake_func_type handshake_func_type;
    typedef stream_socket_core::read_func_type read_func_type;
    typedef stream_socket_core::write_func_type write_func_type;

    enum handshake_type
    {
        client,
        server
    };

public:
    basic_stream_socket(lcy::asio::io_context& ioc, context_type& context) :
        context_(context), impl_(), stream_socket_(ioc)
    {
        lcy::system::error_code ec;
        ssl_stream_socket_core_.construct(impl_, context.native_handle(), ec);
        if ( ec ) {
            throw_ssl_exception(ec, "basic_ssl_stream_socket::basic_ssl_stream_socket");
        }
    }

    ~basic_stream_socket()
    {
        ssl_stream_socket_core_.destroy(impl_, stream_socket_);
    }

    void set_hostname(const std::string& hostname, lcy::system::error_code& ec)
    {
        ssl_stream_socket_core_.set_hostname(impl_, hostname, ec);
    }

    void set_verify_hostname(const std::string& hostname, lcy::system::error_code& ec)
    {
        ssl_stream_socket_core_.set_verify_hostname(impl_, hostname, ec);
    }

    void client_handshake(lcy::system::error_code& ec)
    {
        ssl_stream_socket_core_.client_handshake(impl_, stream_socket_, ec);
    }

    void server_handshake(lcy::system::error_code& ec)
    {
        ssl_stream_socket_core_.server_handshake(impl_, stream_socket_, ec);
    }

    void close(lcy::system::error_code& ec)
    {
        ssl_stream_socket_core_.close(impl_, stream_socket_, ec);
    }

    template <typename MutableBufferView>
    std::size_t read_some(MutableBufferView buffer_view, lcy::system::error_code& ec)
    {
        return ssl_stream_socket_core_.read_some(impl_, stream_socket_, buffer_view, ec);
    }

    template <typename ConstBufferView>
    std::size_t write_some(ConstBufferView buffer_view, lcy::system::error_code& ec)
    {
        return ssl_stream_socket_core_.write_some(impl_, stream_socket_, buffer_view, ec);
    }

    void set_hostname(const std::string& hostname)
    {
        lcy::system::error_code ec;
        set_hostname(hostname, ec);
        if ( ec ) {
            throw_ssl_exception(ec, "basic_ssl_stream_socket::set_hostname");
        }
    }

    void set_verify_hostname(const std::string& hostname)
    {
        lcy::system::error_code ec;
        set_verify_hostname(hostname, ec);
        if ( ec ) {
            throw_ssl_exception(ec, "basic_ssl_stream_socket::set_verify_hostname");
        }
    }

    void client_handshake()
    {
        lcy::system::error_code ec;
        client_handshake(ec);
        if ( ec ) {
            throw_ssl_exception(ec, "basic_ssl_stream_socket::client_handshake");
        }
    }

    void server_handshake()
    {
        lcy::system::error_code ec;
        server_handshake(ec);
        if ( ec ) {
            throw_ssl_exception(ec, "basic_ssl_stream_socket::server_handshake");
        }
    }

    void close()
    {
        lcy::system::error_code ec;
        close(ec);
        if ( ec ) {
            throw_ssl_exception(ec, "basic_ssl_stream_socket::close");
        }
    }

    template <typename MutableBufferView>
    std::size_t read_some(MutableBufferView buffer_view)
    {
        lcy::system::error_code ec;
        std::size_t n = read_some(buffer_view, ec);
        if ( ec ) {
            throw_ssl_exception(ec, "basic_ssl_stream_socket::read_some");
        }
        return n;
    }

    template <typename ConstBufferView>
    std::size_t write_some(ConstBufferView buffer_view)
    {
        lcy::system::error_code ec;
        std::size_t n = write_some(buffer_view, ec);
        if ( ec ) {
            throw_ssl_exception(ec, "basic_ssl_stream_socket::write_some");
        }
        return n;
    }

    void async_client_handshake(handshake_func_type func)
    {
        ssl_stream_socket_core_.async_client_handshake(impl_, stream_socket_, std::move(func));
    }

    void async_server_handshake(handshake_func_type func)
    {
        ssl_stream_socket_core_.async_server_handshake(impl_, stream_socket_, std::move(func));
    }

    template <typename MutableBufferView>
    void async_read_some(MutableBufferView buffer_view, read_func_type func)
    {
        ssl_stream_socket_core_.async_read_some(impl_, stream_socket_, buffer_view, std::move(func));
    }

    template <typename ConstBufferView>
    void async_write_some(ConstBufferView buffer_view, write_func_type func)
    {
        ssl_stream_socket_core_.async_write_some(impl_, stream_socket_, buffer_view, std::move(func));
    }

    context_type& get_context()
    {
        return context_;
    }

    lcy::asio::ip::tcp::socket& get_next_layer()
    {
        return stream_socket_;
    }

private:
    typedef stream_socket_core::ssl_socket_impl ssl_socket_impl;

    context_type& context_;
    ssl_socket_impl impl_;
    lcy::asio::ip::tcp::socket stream_socket_;
    stream_socket_core ssl_stream_socket_core_;
};

}   // namespace detail
}   // namespace ssl
}   // namespace asio
}   // namespace lcy

#endif  // __LCY_ASIO_SSL_DETAIL_BASIC_STREAM_SOCKET_HPP__
