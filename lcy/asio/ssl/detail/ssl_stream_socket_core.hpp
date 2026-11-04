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
        SSL* ssl_;
        BIO* read_bio_;
        BIO* write_bio_;
        char* buffer_;
        std::size_t length_;
    };

    enum { buffer_length = 16384 };

public:
    class handshake_operation;
    class read_operation;
    class write_operation;

public:
    void construct(ssl_socket_impl& impl, SSL_CTX* ssl_ctx, lcy::system::error_code& ec)
    {
        impl.ssl_ = ::SSL_new(ssl_ctx);
        impl.read_bio_ = ::BIO_new(BIO_s_mem());
        impl.write_bio_ = ::BIO_new(BIO_s_mem());
        impl.buffer_ = new char[buffer_length]{ 0 };
        impl.length_ = buffer_length;

        if ( !impl.ssl_ || !impl.read_bio_ || !impl.write_bio_ ) {
            if ( impl.ssl_ ) {
                ::SSL_free(impl.ssl_);
            }
            if ( impl.read_bio_ ) {
                ::BIO_free(impl.read_bio_);
            }
            if ( impl.write_bio_ ) {
                ::BIO_free(impl.write_bio_);
            }
            if ( impl.buffer_ ) {
                delete[] impl.buffer_;
            }

            impl.ssl_ = 0;
            impl.read_bio_ = 0;
            impl.write_bio_ = 0;
            impl.buffer_ = 0;
            impl.length_ = 0;

            ec = make_ssl_error_code(lcy::asio::ssl::error::openssl_error);
            return;
        }
        ::SSL_set_bio(impl.ssl_, impl.read_bio_, impl.write_bio_);

        ec = lcy::system::error_code();
    }

    void destroy(ssl_socket_impl& impl)
    {
        if ( impl.ssl_ ) {
            ::SSL_free(impl.ssl_);
        }
        if ( impl.buffer_ ) {
            delete[] impl.buffer_;
        }

        impl.ssl_ = 0;
        impl.read_bio_ = 0;
        impl.write_bio_ = 0;
        impl.buffer_ = 0;
        impl.length_ = 0;
    }

    void client_handshake(ssl_socket_impl& impl,
        lcy::asio::ip::tcp::socket& stream_socket,
        lcy::system::error_code& ec)
    {
        handshake(impl, stream_socket, ec, true);
    }

    void server_handshake(ssl_socket_impl& impl,
        lcy::asio::ip::tcp::socket& stream_socket,
        lcy::system::error_code& ec)
    {
        handshake(impl, stream_socket, ec, false);
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

    void async_server_handshake(ssl_socket_impl& impl,
        lcy::asio::ip::tcp::socket& stream_socket,
        handshake_func_type func)
    {

    }

    void async_client_handshake(ssl_socket_impl& impl,
        lcy::asio::ip::tcp::socket& stream_socket,
        handshake_func_type func)
    {

    }

    template <typename MutableBufferView>
    void async_read_some(ssl_socket_impl& impl,
        lcy::asio::ip::tcp::socket& stream_socket,
        MutableBufferView buffer_view, read_func_type func)
    {

    }

    template <typename ConstBufferView>
    void async_write_some(ssl_socket_impl& impl,
        lcy::asio::ip::tcp::socket& stream_socket,
        ConstBufferView buffer_view, read_func_type func)
    {

    }

private:
    void handshake(ssl_socket_impl& impl,
        lcy::asio::ip::tcp::socket& stream_socket,
        lcy::system::error_code& ec, bool is_client)
    {
        for ( ;; ) {
            int ret = is_client ? ::SSL_connect(impl.ssl_)
                :SSL_accept(impl.ssl_);

            if ( ret == 1 ) {
                ec = lcy::system::error_code();
                return;
            }

            int ssl_error = ::SSL_get_error(impl.ssl_, ret);

            if ( ssl_error == SSL_ERROR_WANT_READ ) {
                auto view = lcy::asio::buffer_view(impl.buffer_, impl.length_);
                std::size_t n = stream_socket.read_some(view, ec);
                if ( ec ) {
                    return;
                }

                if ( n == 0 ) {
                    ec = make_ssl_error_code(lcy::asio::ssl::error::stream_truncated);
                    return;
                }

                ::BIO_write(impl.read_bio_, impl.buffer_, static_cast<int>(n));

                continue;
            }

            if ( ssl_error == SSL_ERROR_WANT_WRITE ) {
                int n = ::BIO_read(impl.write_bio_, impl.buffer_, impl.length_);
                if ( n <= 0 ) {
                    ec = make_ssl_error_code(lcy::asio::ssl::error::openssl_error);
                    return;
                }

                auto view = lcy::asio::buffer_view(impl.buffer_, n);
                for ( ;; ) {
                    std::size_t nsend = stream_socket.write_some(view, ec);
                    if ( ec ) {
                        return;
                    }

                    if ( nsend < view.length() ) {
                        view = view + nsend;
                        continue;
                    }

                    break;
                }

                continue;
            }

            ec = map_ssl_error(impl.ssl_, ret);
            return;
        }
    }
};

}   // namespace detail
}   // namespace ssl
}   // namespace asio
}   // namespace lcy

#endif  // __LCY_ASIO_SSL_STREAM_SOCKET_CORE_HPP__