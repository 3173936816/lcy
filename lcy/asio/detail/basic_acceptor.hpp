#ifndef __LCY_ASIO_DETAIL_BASIC_ACCEPTOR_HPP__
#define __LCY_ASIO_DETAIL_BASIC_ACCEPTOR_HPP__

#include "lcy/noncopyable.hpp"
#include "lcy/asio/detail/basic_stream_socket.hpp"

namespace lcy
{
namespace asio
{
namespace detail
{

template <
    typename Executor,
    typename Protocol,
    typename Endpoint
>
class basic_acceptor :
    private noncopyable
{
public:
    typedef basic_stream_socket<
        Executor,
        Protocol,
        Endpoint
    > stream_socket_type;

    typedef Executor executor_type;
    typedef Protocol protocol_type;
    typedef Endpoint endpoint_type;

    typedef typename stream_socket_type::accept_func_type accept_func_type;
    typedef typename stream_socket_type::native_handle_type native_handle_type;

public:
    explicit basic_acceptor(executor_type& executor) :
        executor_(executor), stream_socket_(executor) {}

    template <typename... SocketOptions>
    basic_acceptor(executor_type& executor,
                   endpoint_type endpoint,
                   SocketOptions... options) :
        executor_(executor), stream_socket_(executor)
    {
        stream_socket_.open(endpoint.protocol());
        set_options(options...);
        stream_socket_.bind(endpoint);
        stream_socket_.listen(SOMAXCONN);
    }

    native_handle_type native_handle()
    {
        return stream_socket_.native_handle();
    }

    bool is_open() const
    {
        return stream_socket_.is_open();
    }

    void open(protocol_type proto)
    {
        stream_socket_.open(proto);
    }

    void close()
    {
        stream_socket_.close();
    }

    void bind(endpoint_type endpoint)
    {
        stream_socket_.bind(endpoint);
    }

    void listen(int backlog)
    {
        stream_socket_.listen(backlog);
    }

    void accept(stream_socket_type& target_socket, endpoint_type& endpoint)
    {
        stream_socket_.accept(target_socket, endpoint);
    }

    template <typename SocketOption>
    void set_option(SocketOption opt)
    {
        stream_socket_.set_option(opt);
    }

    template <typename SocketOption>
    void get_option(SocketOption& opt)
    {
        stream_socket_.get_option(opt);
    }

    void local_endpoint(endpoint_type& endpoint)
    {
        stream_socket_.local_endpoint(endpoint);
    }

    void cancel()
    {
        stream_socket_.cancel();
    }

    void open(protocol_type proto, lcy::system::error_code& ec)
    {
        stream_socket_.open(proto, ec);
    }

    void close(lcy::system::error_code& ec)
    {
        stream_socket_.close(ec);
    }

    void bind(endpoint_type endpoint, lcy::system::error_code& ec)
    {
        stream_socket_.bind(endpoint, ec);
    }

    void listen(int backlog, lcy::system::error_code& ec)
    {
        stream_socket_.listen(backlog, ec);
    }

    void accept(stream_socket_type& target_socket,
        endpoint_type& endpoint, lcy::system::error_code& ec)
    {
        stream_socket_.accept(target_socket, endpoint, ec);
    }

    template <typename SocketOption>
    void set_option(SocketOption opt, lcy::system::error_code& ec)
    {
        stream_socket_.set_option(opt, ec);
    }

    template <typename SocketOption>
    void get_option(SocketOption& opt, lcy::system::error_code& ec)
    {
        stream_socket_.get_option(opt, ec);
    }

    void local_endpoint(endpoint_type& endpoint, lcy::system::error_code& ec)
    {
        stream_socket_.local_endpoint(endpoint, ec);
    }

    void cancel(lcy::system::error_code& ec)
    {
        stream_socket_.cancel(ec);
    }

    void async_accept(stream_socket_type& target_socket,
        endpoint_type& endpoint, accept_func_type func)
    {
        stream_socket_.async_accept(target_socket, endpoint, std::move(func));
    }

    executor_type& get_executor()
	{
		return executor_;
	}

private:
    void set_options() {}

    template <typename SocketOption, typename... Args>
    void set_options(SocketOption opt, Args... rest)
    {
        set_option(opt);
        set_options(rest...);
    }

private:
    executor_type& executor_;
    stream_socket_type stream_socket_;
};

}   // namespace detail
}   // namespace asio
}   // namespace lcy

#endif  // __LCY_ASIO_DETAIL_BASIC_ACCEPTOR_HPP__