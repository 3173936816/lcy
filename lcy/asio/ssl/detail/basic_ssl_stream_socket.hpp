#ifndef __LCY_ASIO_SSL_DETAIL_BASIC_SSL_STREAM_SOCKET_HPP__
#define __LCY_ASIO_SSL_DETAIL_BASIC_SSL_STREAM_SOCKET_HPP__

#include <cstddef>
#include <functional>

#include <openssl/ssl.h>
#include <openssl/bio.h>

#include "lcy/noncopyable.hpp"
#include "lcy/asio/ip/tcp.hpp"
#include "lcy/asio/buffer_view.hpp"
#include "lcy/asio/ssl/detail/error.hpp"
#include "lcy/asio/ssl/detail/throw_ssl_exception.hpp"
#include "lcy/asio/ssl/detail/ssl_stream_socket_core.hpp"

namespace lcy
{
namespace asio
{
namespace ssl
{
namespace detail
{

template <typename Context>
class basic_ssl_stream_socket :
    private noncopyable
{
public:
    typedef Context context_type;

    enum handshake_type
    { 
        client,
        server 
    };

public:
    basic_ssl_stream_socket(lcy::asio::io_context& ioc, context_type& context) :
        context_(context), stream_socket_(ioc)
    {
        lcy::system::error_code ec;
        ssl_stream_socket_core_.construct(impl_, context.native_handle(), ec);
        if ( ec ) {
            throw_ssl_exception(ec, "basic_ssl_stream_socket::basic_ssl_stream_socket");
        }
    }

    ~basic_ssl_stream_socket()
    {
    }

    void client_handshake(lcy::system::error_code& ec)
    {
        
    }

    void server_handshake(lcy::system::error_code& ec)
    {

    }

    void close(lcy::system::error_code& ec)
    {

    }

    template <typename MutableBufferView>
    std::size_t read_some(MutableBufferView buffer_view, lcy::system::error_code& ec)
    {

    }

    template <typename ConstBufferView>
    std::size_t write_some(ConstBufferView buffer_view, lcy::system::error_code& ec)
    {

    }

    void client_handshake()
    {
        
    }

    void server_handshake()
    {
        
    }

    void close()
    {

    }

    template <typename MutableBufferView>
    std::size_t read_some(MutableBufferView buffer_view)
    {

    }

    template <typename ConstBufferView>
    std::size_t write_some(ConstBufferView buffer_view)
    {

    }

    void async_client_handshake(handshake_func_type func)
    {

    }

    void async_server_handshake(handshake_func_type func)
    {
        
    }

    template <typename MutableBufferView>
    std::size_t async_read_some(MutableBufferView buffer_view, read_func_type func)
    {

    }

    template <typename ConstBufferView>
    std::size_t async_write_some(ConstBufferView buffer_view, read_func_type func)
    {

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
    typedef ssl_stream_socket_core::ssl_socket_impl ssl_socket_impl;

    context_type& context_;
    ssl_socket_impl impl_;
    lcy::asio::ip::tcp::socket stream_socket_;
    ssl_stream_socket_core ssl_stream_socket_core_;
};

}   // namespace detail
}   // namespace ssl
}   // namespace asio
}   // namespace lcy

#endif  // __LCY_ASIO_SSL_DETAIL_BASIC_SSL_STREAM_SOCKET_HPP__