#ifndef __LCY_ASIO_SSL_DETAIL_STREAM_SOCKET_CORE_HPP__
#define __LCY_ASIO_SSL_DETAIL_STREAM_SOCKET_CORE_HPP__

#include <cstddef>
#include <memory>
#include <functional>
#include <openssl/ssl.h>
#include <openssl/bio.h>

#include "lcy/noncopyable.hpp"
#include "lcy/system/error.hpp"
#include "lcy/asio/io_context.hpp"
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

class stream_socket_core :
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

    void destroy(ssl_socket_impl& impl, lcy::asio::ip::tcp::socket& stream_socket)
    {
        if ( stream_socket.is_open() ) {
            lcy::system::error_code ignored_ec;
            this->close(impl, stream_socket, ignored_ec);
        }

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

    void set_hostname(ssl_socket_impl& impl,
        const std::string& hostname, lcy::system::error_code& ec)
    {
        int ret = ::SSL_set_tlsext_host_name(impl.ssl_, hostname.c_str());
        if ( ret != 1 ) {
            ec = map_ssl_error(impl.ssl_, ret);
            return;
        }

        ec = lcy::system::error_code();
    }

    void set_verify_hostname(ssl_socket_impl& impl,
        const std::string& hostname, lcy::system::error_code& ec)
    {
        int ret = ::SSL_set1_host(impl.ssl_, hostname.c_str());
        if ( ret != 1 ) {
            ec = make_ssl_error_code(lcy::asio::ssl::error::openssl_error);
            return;
        }

        ec = lcy::system::error_code();
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
        if ( impl.ssl_ ) {
            ::SSL_shutdown(impl.ssl_);
        }

        lcy::system::error_code ignore_ec;
        flush_write_bio(impl, stream_socket, ignore_ec);

        stream_socket.close(ec);
    }

    template <typename MutableBufferView>
    std::size_t read_some(ssl_socket_impl& impl,
        lcy::asio::ip::tcp::socket& stream_socket,
        MutableBufferView buffer_view, lcy::system::error_code& ec)
    {
        if ( !stream_socket.is_open() ) {
            ec = lcy::system::error_code(
                lcy::system::error::bad_file_descriptor);
            return 0;
        }

        for ( ;; ) {
            int ret = ::SSL_read(impl.ssl_, buffer_view.data(),
                static_cast<int>(buffer_view.length()));

            if ( ret > 0 ) {
                ec = lcy::system::error_code();
                return ret;
            }

            int ssl_error = ::SSL_get_error(impl.ssl_, ret);

            if ( ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE ) {
                int pending = ::BIO_ctrl_pending(impl.write_bio_);
                if ( pending > 0 ) {
                    if ( !flush_write_bio(impl, stream_socket, ec) ) {
                        return 0;
                    }
                }

                if ( !fill_read_bio(impl, stream_socket, ec) ) {
                    return 0;
                }
                continue;
            }

            ec = map_ssl_error(impl.ssl_, ret);

            return 0;
        }
    }

    template <typename ConstBufferView>
    std::size_t write_some(ssl_socket_impl& impl,
        lcy::asio::ip::tcp::socket& stream_socket,
        ConstBufferView buffer_view, lcy::system::error_code& ec)
    {
        if ( !stream_socket.is_open() ) {
            ec = lcy::system::error_code(
                lcy::system::error::bad_file_descriptor);
            return 0;
        }

        for ( ;; ) {
            int ret = ::SSL_write(impl.ssl_, buffer_view.data(),
                static_cast<int>(buffer_view.length()));

            if ( ret > 0 ) {
                if ( !flush_write_bio(impl, stream_socket, ec) ) {
                    return 0;
                }

                ec = lcy::system::error_code();
                return static_cast<std::size_t>(ret);
            }

            int ssl_error = ::SSL_get_error(impl.ssl_, ret);

            if ( ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE ) {
                int pending = ::BIO_ctrl_pending(impl.write_bio_);
                if ( pending > 0 ) {
                    if ( !flush_write_bio(impl, stream_socket, ec) ) {
                        return 0;
                    }
                }

                if ( !fill_read_bio(impl, stream_socket, ec) ) {
                    return 0;
                }
                continue;
            }

            ec = map_ssl_error(impl.ssl_, ret);

            return 0;
        }
    }

    void async_client_handshake(ssl_socket_impl& impl,
        lcy::asio::ip::tcp::socket& stream_socket,
        handshake_func_type func)
    {
        async_handshake(impl, stream_socket, true, std::move(func));
    }

    void async_server_handshake(ssl_socket_impl& impl,
        lcy::asio::ip::tcp::socket& stream_socket,
        handshake_func_type func)
    {
        async_handshake(impl, stream_socket, false, std::move(func));
    }

    template <typename MutableBufferView>
    void async_read_some(ssl_socket_impl& impl,
        lcy::asio::ip::tcp::socket& stream_socket,
        MutableBufferView buffer_view, read_func_type func)
    {
        if ( !stream_socket.is_open() ) {
            lcy::system::error_code ec(
                lcy::system::error::bad_file_descriptor);
            lcy::asio::post(stream_socket.get_executor(),
                [func, ec]() {
                    func(ec, 0);
                });
            return;
        }

        int ret = ::SSL_read(impl.ssl_, buffer_view.data(),
            static_cast<int>(buffer_view.length()));

        if ( ret > 0 ) {
            lcy::asio::post(stream_socket.get_executor(),
                [func, ret]() {
                    lcy::system::error_code ec;
                    func(ec, static_cast<std::size_t>(ret));
                });
            return;
        }

        int ssl_error = ::SSL_get_error(impl.ssl_, ret);

        if ( ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE ) {
            async_drain(impl, stream_socket,
                [func](lcy::system::error_code ec) {
                    func(ec, 0);
                },
                [&impl, &stream_socket, buffer_view, func, this]() {
                    async_read_some(impl, stream_socket, buffer_view, func);
                });
            return;
        }

        lcy::system::error_code ec = map_ssl_error(impl.ssl_, ret);
        lcy::asio::post(stream_socket.get_executor(),
            [func, ec]() {
                func(ec, 0);
            });
    }

    template <typename ConstBufferView>
    void async_write_some(ssl_socket_impl& impl,
        lcy::asio::ip::tcp::socket& stream_socket,
        ConstBufferView buffer_view, write_func_type func)
    {
        if ( !stream_socket.is_open() ) {
            lcy::system::error_code ec(
                lcy::system::error::bad_file_descriptor);
            lcy::asio::post(stream_socket.get_executor(),
                [func, ec]() {
                    func(ec, 0);
                });
            return;
        }

        int ret = ::SSL_write(impl.ssl_, buffer_view.data(),
            static_cast<int>(buffer_view.length()));

        if ( ret > 0 ) {
            async_flush_write_bio(impl, stream_socket,
                [func](lcy::system::error_code ec) {
                    func(ec, 0);
                },
                [func, ret]() {
                    lcy::system::error_code ec;
                    func(ec, static_cast<std::size_t>(ret));
                });
            return;
        }

        int ssl_error = ::SSL_get_error(impl.ssl_, ret);

        if ( ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE ) {
            async_drain(impl, stream_socket,
                [func](lcy::system::error_code ec) {
                    func(ec, 0);
                },
                [&impl, &stream_socket, buffer_view, func, this]() {
                    async_write_some(impl, stream_socket, buffer_view, func);
                });
            return;
        }

        lcy::system::error_code ec = map_ssl_error(impl.ssl_, ret);
        lcy::asio::post(stream_socket.get_executor(),
            [func, ec]() {
                func(ec, 0);
            });
    }

private:
    typedef std::function<
        void (
            lcy::system::error_code ec
        )
    > error_func_type;

    typedef std::function<void()> next_func_type;

    bool fill_read_bio(ssl_socket_impl& impl,
        lcy::asio::ip::tcp::socket& stream_socket,
        lcy::system::error_code& ec)
    {
        auto view = lcy::asio::buffer_view(impl.buffer_, impl.length_);
        std::size_t n = stream_socket.read_some(view, ec);
        if ( ec ) {
            return false;
        }

        if ( n == 0 ) {
            ec = make_ssl_error_code(lcy::asio::ssl::error::stream_truncated);
            return false;
        }

        ::BIO_write(impl.read_bio_, impl.buffer_, static_cast<int>(n));

        return true;
    }

    bool flush_write_bio(ssl_socket_impl& impl,
        lcy::asio::ip::tcp::socket& stream_socket,
        lcy::system::error_code& ec)
    {
        for ( ;; ) {
            int n = ::BIO_read(impl.write_bio_, impl.buffer_, impl.length_);
            if ( n <= 0 ) {
                break;
            }

            auto view = lcy::asio::buffer_view(impl.buffer_, static_cast<std::size_t>(n));
            for ( ;; ) {
                std::size_t nsend = stream_socket.write_some(view, ec);
                if ( ec ) {
                    return false;
                }

                if ( nsend == 0 ) {
                    ec = make_ssl_error_code(lcy::asio::ssl::error::stream_truncated);
                    return false;
                }

                if ( nsend == view.length() ) {
                    break;
                }

                view = view + nsend;
            }
        }

        return true;
    }

    void handshake(ssl_socket_impl& impl,
        lcy::asio::ip::tcp::socket& stream_socket,
        lcy::system::error_code& ec, bool is_client)
    {
        if ( !stream_socket.is_open() ) {
            ec = lcy::system::error_code(
                lcy::system::error::bad_file_descriptor);
            return;
        }

        for ( ;; ) {
            int ret = is_client ? ::SSL_connect(impl.ssl_)
                : ::SSL_accept(impl.ssl_);

            if ( ret == 1 ) {
                ec = lcy::system::error_code();
                return;
            }

            int ssl_error = ::SSL_get_error(impl.ssl_, ret);

            if ( ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE ) {
                int pending = ::BIO_ctrl_pending(impl.write_bio_);
                if ( pending > 0 ) {
                    if ( !flush_write_bio(impl, stream_socket, ec) ) {
                        return;
                    }
                }

                if ( !fill_read_bio(impl, stream_socket, ec) ) {
                    return;
                }
                continue;
            }

            ec = map_ssl_error(impl.ssl_, ret);

            return;
        }
    }

    void async_fill_read_bio(ssl_socket_impl& impl,
        lcy::asio::ip::tcp::socket& stream_socket,
        error_func_type error_func, next_func_type next_func)
    {
        auto view = lcy::asio::buffer_view(impl.buffer_, impl.length_);
        stream_socket.async_read_some(view,
            [&impl, &stream_socket, error_func, next_func](
                lcy::system::error_code ec, std::size_t n) {
            if ( ec ) {
                error_func(ec);
                return;
            }

            if ( n == 0 ) {
                ec = make_ssl_error_code(lcy::asio::ssl::error::stream_truncated);
                error_func(ec);
                return;
            }

            ::BIO_write(impl.read_bio_, impl.buffer_, static_cast<int>(n));

            next_func();
        });
    }

    void async_flush_write_bio(ssl_socket_impl& impl,
        lcy::asio::ip::tcp::socket& stream_socket,
        error_func_type error_func, next_func_type next_func)
    {
        int n = ::BIO_read(impl.write_bio_, impl.buffer_, impl.length_);
        if ( n <= 0 ) {
            lcy::asio::post(stream_socket.get_executor(),
                [next_func]() {
                    next_func();
                });
            return;
        }

        auto view = lcy::asio::buffer_view(impl.buffer_, static_cast<std::size_t>(n));
        async_write_loop(impl, stream_socket, view, static_cast<std::size_t>(n), 0, error_func,
            [&impl, &stream_socket, error_func, next_func, this]() {
                async_flush_write_bio(impl, stream_socket, error_func, next_func);
            });
    }

    template <typename ConstBufferView>
    void async_write_loop(ssl_socket_impl& impl,
        lcy::asio::ip::tcp::socket& stream_socket,
        ConstBufferView view, std::size_t total, std::size_t written,
        error_func_type error_func, next_func_type next_func)
    {
        if ( written == total ) {
            lcy::asio::post(stream_socket.get_executor(),
                [next_func]() {
                    next_func();
                });
            return;
        }

        auto part = lcy::asio::const_buffer_view(
            static_cast<const char*>(view.data()) + written, total - written);

        stream_socket.async_write_some(part,
            [&impl, &stream_socket, view, total, written, error_func, next_func, this](
                lcy::system::error_code ec, std::size_t nsend) {
            if ( ec ) {
                error_func(ec);
                return;
            }

            if ( nsend == 0 ) {
                ec = make_ssl_error_code(lcy::asio::ssl::error::stream_truncated);
                error_func(ec);
                return;
            }

            async_write_loop(impl, stream_socket, view,
                total, written + nsend, error_func, next_func);
        });
    }

    void async_drain_flush(ssl_socket_impl& impl,
        lcy::asio::ip::tcp::socket& stream_socket,
        error_func_type error_func, next_func_type next_func)
    {
        async_flush_write_bio(impl, stream_socket, error_func,
            [&impl, &stream_socket, error_func, next_func, this]() {
            async_fill_read_bio(impl, stream_socket, error_func, next_func);
        });
    }

    void async_drain(ssl_socket_impl& impl,
        lcy::asio::ip::tcp::socket& stream_socket,
        error_func_type error_func, next_func_type next_func)
    {
        int pending = ::BIO_ctrl_pending(impl.write_bio_);
        if ( pending > 0 ) {
            async_drain_flush(impl, stream_socket, error_func, next_func);
            return;
        }

        async_fill_read_bio(impl, stream_socket, error_func, next_func);
    }

    void async_handshake(ssl_socket_impl& impl,
        lcy::asio::ip::tcp::socket& stream_socket,
        bool is_client, handshake_func_type func)
    {
        if ( !stream_socket.is_open() ) {
            lcy::system::error_code ec(
                lcy::system::error::bad_file_descriptor);
            lcy::asio::post(stream_socket.get_executor(),
                [func, ec]() {
                    func(ec);
                });
            return;
        }

        int ret = is_client ? ::SSL_connect(impl.ssl_)
            : ::SSL_accept(impl.ssl_);

        if ( ret == 1 ) {
            lcy::asio::post(stream_socket.get_executor(),
                [func]() {
                    lcy::system::error_code ec;
                    func(ec);
                });
            return;
        }

        int ssl_error = ::SSL_get_error(impl.ssl_, ret);

        if ( ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE ) {
            async_drain(impl, stream_socket, func,
                    [&impl, &stream_socket, is_client, func, this]() {
                async_handshake(impl, stream_socket, is_client, func);
            });
            return;
        }

        lcy::system::error_code ec = map_ssl_error(impl.ssl_, ret);
        lcy::asio::post(stream_socket.get_executor(),
            [func, ec]() {
                func(ec);
            });
    }
};

}   // namespace detail
}   // namespace ssl
}   // namespace asio
}   // namespace lcy

#endif  // __LCY_ASIO_SSL_DETAIL_STREAM_SOCKET_CORE_HPP__