#ifndef __LCY_ASIO_SSL_STREAM_SOCKET_CORE_HPP__
#define __LCY_ASIO_SSL_STREAM_SOCKET_CORE_HPP__

#include <cstddef>
#include <memory>
#include <functional>
#include <openssl/ssl.h>
#include <openssl/bio.h>

#include "lcy/noncopyable.hpp"
#include "lcy/system/error.hpp"
#include "lcy/asio/ip/tcp.hpp"
#include "lcy/asio/buffer_view.hpp"
#include "lcy/asio/ssl/detail/error.hpp"

namespace lcy
{
namespace asio
{
namespace ssl
{
namespace detail
{

class ssl_stream_socket_core :
    private noncopyable
{
public:
    typedef std::function<
        void (
            lcy::system::error_code ec
        )
    > handshake_func_type;

    typedef lcy::asio::ip::tcp::socket::read_func_type read_func_type;
    typedef lcy::asio::ip::tcp::socket::write_func_type write_func_type;

public:
    struct ssl_socket_impl
    {
        SSL_CTX* ssl_ctx_;
        SSL* ssl_;
        BIO* read_bio_;
        BIO* write_bio_;
    };

public:
    class handshake_operation;
    class read_operation;
    class write_operation;

public:
    void construct(ssl_socket_impl& impl, SSL_CTX* ssl_ctx, lcy::system::error_code& ec)
    {
        impl.ssl_ctx_ = ssl_ctx;
        impl.ssl_ = ::SSL_new(context.native_handle());
        impl.read_bio_ = ::BIO_new(BIO_s_mem());
        impl.write_bio_ = ::BIO_new(BIO_s_mem());

        if ( !impl.ssl_ctx_ || !impl.ssl_ || !impl.read_bio_ || !impl.write_bio_ ) {
            ec = make_ssl_error_code(lcy::asio::ssl::error::openssl_error);
            return;
        }
        ::SSL_set_bio(ssl_, read_bio_, write_bio_);
    }

    void destroy(ssl_socket_impl& impl)
    {
        if ( impl_.ssl_ ) {
            ::SSL_free(impl_.ssl_);
        }

        impl_.ssl_ctx_ = 0;
        impl_.ssl_ = 0;
        read_bio_ = 0;
        write_bio_ = 0;
    }

    void handshake(ssl_socket_impl& impl,
        lcy::asio::ip::tcp::socket& stream_socket,
        lcy::system::error_code& ec)
    {
        
    }

    void close(ssl_socket_impl& impl,
        lcy::asio::ip::tcp::socket& stream_socket,
        lcy::system::error_code& ec)
    {

    }

    template <typename MutableBufferView>
    std::size_t read_some(ssl_socket_impl& impl,
        lcy::asio::ip::tcp::socket& stream_socket,
        MutableBufferView buffer_view, lcy::system::error_code& ec)
    {

    }

    template <typename ConstBufferView>
    std::size_t write_some(ssl_socket_impl& impl,
        lcy::asio::ip::tcp::socket& stream_socket,
        ConstBufferView buffer_view, lcy::system::error_code& ec)
    {

    }

    template <typename MutableBufferView>
    std::size_t async_read_some(ssl_socket_impl& impl,
        lcy::asio::ip::tcp::socket& stream_socket,
        MutableBufferView buffer_view, read_func_type func)
    {

    }

    template <typename ConstBufferView>
    std::size_t async_write_some(ssl_socket_impl& impl,
        lcy::asio::ip::tcp::socket& stream_socket,
        ConstBufferView buffer_view, read_func_type func)
    {

    }
};

}   // namespace detail
}   // namespace ssl
}   // namespace asio
}   // namespace lcy

#endif  // __LCY_ASIO_SSL_STREAM_SOCKET_CORE_HPP__