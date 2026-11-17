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
    typedef TcpSocket socket_type;
    typedef SinkBase base_type;
    typedef std::shared_ptr<basic_tcp_sink> sptr_type;
    typedef typename base_type::record_sptr_type record_sptr_type;

public:
    basic_tcp_sink(lcy::asio::io_context& ioc) :
        state_(state::disconnected), ioc_(ioc), strand_(ioc), tcp_socket_(ioc)
    {
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
        closing,
        disconnected,
    };

    void sink_state_machine(record_sptr_type rsptr)
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
            case state::closing : {
                break;
            }
            case state::disconnected : {
                break;
            }
        }
    }

private:
    state state_;
    lcy::asio::io_context& ioc_;
    lcy::asio::strand strand_;
    socket_type tcp_socket_;
    std::size_t max_deque_length_;
    std::list<std::string> message_deque_;
};

}   // namespace detail
}   // namespace log4cc
}   // namespace lcy

#endif // __LCY_LOG4CC_DETAIL_BASIC_TCP_SINK_HPP__