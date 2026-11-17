#ifndef __LCY_LOG4CC_DETAIL_BASIC_TCP_SINK_HPP__
#define __LCY_LOG4CC_DETAIL_BASIC_TCP_SINK_HPP__

#include <list>
#include <memory>
#include <string>

#include "lcy/asio/strand.hpp"
#include "lcy/asio/ip/tcp.hpp"

namespace lcy
{
namespace log4cc
{
namespace detail
{

template <
    typename TcpSocket,
    typename SinkBase
>
class basic_tcp_sink :
    public SinkBase
{
public:
    typedef std::function<
        void (
            lcy::system::error_code& ec
        )
    > error_func_type;

    typedef std::function<
        void (
            std::string& format_message
        )
    > per_record_func_type;

    typedef TcpSocket socket_type;
    typedef SinkBase base_type;

    typedef std::shared_ptr<basic_tcp_sink> sptr_type;
    typedef typename base_type::record_sptr_type record_sptr_type;

    typedef typename socket_type::endpoint_type endpoint_type;

public:
    basic_tcp_sink(lcy::asio::io_context& ioc, endpoint_type endpoint) :
        state_(state::disconnected), ioc_(ioc),
        endpoint_(endpoint), strand_(ioc), tcp_socket_(ioc)
    {
        execute_state_machine(0);
    }

    void reload() override
    {
    }

    void flush() override
    {
    }

    void log(record_sptr_type rsptr) override
    {
    }

private:
    enum state
    {
        connecting,
        connected,
        sending,
        idle,
        closing,
        disconnected,
    };

    void execute_state_machine(record_sptr_type rsptr)
    {
        switch ( state_ ) {
            case state::connecting : {
                break;
            }
            case state::connected : {
                break;
            }
            case state::sending : {
                break;
            }
            case state::idle : {
                break;
            }
            case state::closing : {
                break;
            }
            case state::disconnected : {
                state_ = state::connecting;
                do_async_connect();
                break;
            }
        }
    }

    void do_async_connect()
    {
    }

private:
    state state_;
    lcy::asio::io_context& ioc_;
    endpoint_type endpoint_;
    socket_type tcp_socket_;
    lcy::asio::strand strand_;
    std::size_t max_deque_length_;
    std::list<std::string> message_deque_;

    error_func_type error_func_;
    per_record_func_type per_record_func_;
};

}   // namespace detail
}   // namespace log4cc
}   // namespace lcy

#endif // __LCY_LOG4CC_DETAIL_BASIC_TCP_SINK_HPP__