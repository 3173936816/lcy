#ifndef __LCY_ASIO_SSL_DETAIL_BASIC_STREAM_SOCKET_HPP__
#define __LCY_ASIO_SSL_DETAIL_BASIC_STREAM_SOCKET_HPP__

#include <cstddef>
#include <memory>
#include <functional>

#include <openssl/ssl.h>
#include <openssl/bio.h>

#include "lcy/noncopyable.hpp"
#include "lcy/system/error.hpp"
#include "lcy/asio/ip/tcp.hpp"
#include "lcy/asio/ssl/context.hpp"
#include "lcy/asio/ssl/error.hpp"
#include "lcy/asio/ssl/detail/error.hpp"

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
    basic_stream_socket(lcy::asio::io_context& ioc, Context& context) :
        context_(context),
        ssl_(::SSL_new(context.native_handle())),
        read_bio_(::BIO_new(BIO_s_mem())),
        write_bio_(::BIO_new(BIO_s_mem())),
        stream_socket_(ioc)
    {
        if ( !context_.native_handle() || !read_bio_ || !write_bio_ ) {
            lcy::system::error_code ec =
                make_ssl_error_code(lcy::asio::ssl::error::openssl_error);
            throw_ssl_exception(ec, "basic_stream_socket::basic_stream_socket");
        }
        ::SSL_set_bio(context_.native_handle(), read_bio_, write_bio_);
    }

    void is_open()
    {
        return stream_socket_.is_open();
    }

    void close(lcy::system::error_code& ec)
    {
        stream_socket_.close(ec);
    }

    void cancel(lcy::system::error_code& ec)
    {
        stream_socket_.cancel(ec);
    }

    void connect(lcy::asio::ip::tcp::endpoint endpoint, lcy::system::error_code& ec)
    { 
        stream_socket_.connect(endpoint, ec);
    }

    void bind(lcy::asio::ip::tcp::endpoint endpoint, lcy::system::error_code& ec)
    { 
        stream_socket_.bind(endpoint, ec);
    }

    template <typename SocketOption>
    void set_option(SocketOption option, lcy::system::error_code& ec)
    {
        stream_socket_.set_option(option, ec);
    }

    template <typename SocketOption>
    void get_option(SocketOption& option, lcy::system::error_code& ec)
    {
        stream_socket_.get_option(option, ec);
    }

    void local_endpoint(lcy::asio::ip::tcp::endpoint& endpoint, lcy::system::error_code& ec)
	{
		stream_socket_.local_endpoint(endpoint, ec);
	}

	void remote_endpoint(lcy::asio::ip::tcp::endpoint& endpoint, lcy::system::error_code& ec)
	{
		stream_socket_.remote_endpoint(endpoint, ec);
	}

    void close()
    {
        stream_socket_.close();
    }

    void cancel()
    {
        stream_socket_.cancel();
    }

    void connect(lcy::asio::ip::tcp::endpoint endpoint)
    { 
        stream_socket_.connect(endpoint);
    }

    void bind(lcy::asio::ip::tcp::endpoint endpoint)
    { 
        stream_socket_.bind(endpoint);
    }

    template <typename SocketOption>
    void set_option(SocketOption option)
    {
        stream_socket_.set_option(option);
    }

    template <typename SocketOption>
    void get_option(SocketOption& option)
    {
        stream_socket_.get_option(option);
    }

    void local_endpoint(lcy::asio::ip::tcp::endpoint& endpoint)
	{
		stream_socket_.local_endpoint(endpoint);
	}

	void remote_endpoint(lcy::asio::ip::tcp::endpoint& endpoint)
	{
		stream_socket_.remote_endpoint(endpoint);
	}
    
    Context& get_context()
    {
        return context_;
    } 
    
    lcy::asio::ip::tcp::socket& get_next_layer()
    {
        return stream_socket_;
    }

private:
    Context& context_;
    BIO* read_bio_;
    BIO* write_bio_;
    lcy::asio::ip::tcp::socket stream_socket_;
};

}   // namespace detail
}   // namespace ssl
}   // namespace asio
}   // namespace lcy

#endif  // __LCY_ASIO_SSL_DETAIL_BASIC_STREAM_SOCKET_HPP__