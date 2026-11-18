#ifndef __LCY_LOG4CC_DETAIL_BASIC_TCP_SINK_HPP__
#define __LCY_LOG4CC_DETAIL_BASIC_TCP_SINK_HPP__

#include <list>
#include <memory>
#include <string>

#include "lcy/asio/strand.hpp"
#include "lcy/asio/ip/tcp.hpp"
#include "lcy/asio/async_stream.hpp"

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
    basic_tcp_sink(lcy::asio::io_context& ioc, endpoint_type endpoint, std::size_t max_deque_length) :
        state_(state::initstate), ioc_(ioc), endpoint_(endpoint),
        strand_(ioc), tcp_socket_(ioc), max_deque_length_(max_deque_length)
    {
        strand_.post([this]() {
            execute_state_machine();
        });
    }

    void reload() override
    {
    }

    void flush() override
    {
    }

    void log(record_sptr_type rsptr) override
    {
        if ( base_type::formatter_ ) {
            auto do_start_send_func = [this](){
                if ( message_deque_.size() >= max_deque_length_ ) {
                    lcy::system::error_code ec(lcy::system::error::no_buffer_space);
                    if ( error_func_ ) {
                        error_func_(ec);
                    }
                    return;
                }

                std::string message = base_type::formatter_->format(rsptr);
                if ( per_record_func_ ) {
                    per_record_func_(message);
                }

                message_deque_.push_back(std::move(message));

                if ( state_ == state::idle ) {
                    do_async_send();
                };
            };

            strand_.post(std::move(do_start_send_func));
        }
    }

private:
    enum state
    {
        initstate,
        connecting,
        connected,
        sending,
        idle,
        closing,
    };

    void execute_state_machine()
    {
        switch ( state_ ) {
            case state::initstate : {
                do_sink_init();
                break;
            }
            case state::connecting : {
                do_async_connect();
                break;
            }
            case state::connected : {
                do_start_async_send();
                break;
            }
            case state::sending : {
                do_async_send();
                break;
            }
            case state::idle : {
                break;
            }
            case state::closing : {
                break;
            }
        }
    }

    void do_sink_init()
    {
        auto do_sink_init_func = [this]() {
            lcy::system::error_code ec;
            tcp_socket_.open(endpoint_.protocol(), ec);
            tcp_socket_.set_option(lcy::asio::ip::socket_option::keep_alive(1));
            tcp_socket_.set_option(lcy::asio::ip::socket_option::tcp_nodelay(1));

            if ( ec ) {
                if ( error_func_ ) {
                    error_func_(ec)
                }
                return;
            }

            state_ = state::connecting;
            execute_state_machine();
        };

        strand_.post(std::move(do_sink_init_func));
    }

    void do_async_connect()
    {
        auto on_connected_func = [](lcy::system::error_code ec) {
            if ( ec ) {
                if ( error_func_ ) {
                    error_func_(ec);
                }
                return;
            }

            state_ = state::connected;
            execute_state_machine();
        };

        auto do_async_connect_func = [this, on_connected_func]() {
            tcp_socket_.async_connect(endpoint_,
                lcy::asio::strand_wrapper(std::move(on_connected_func)));
        };

        strand_.post(std::move(do_async_connect_func));
    }

    void do_start_async_send()
    {
        auto do_start_async_send_func = [this]() {
            state_ = message_deque_.empty()
                ? state::idle
                : state::sending;
            execute_state_machine();
        };

        strand_.post(std::move(do_start_async_send_func));
    }

    void do_async_send()
    {
        auto on_one_message_send_completed = [this](lcy::system::error_code ec, std::size_t) {
            if ( ec ) {
                if ( error_func_ ) {
                    error_func_(ec);
                }
                return;
            }

            message_deque_.pop_front();

            state_ = message_deque_.empty()
                ? state::idle
                : state::sending;
            execute_state_machine();
        };

        auto do_async_send_func = [this, on_one_message_send_completed]() {
            std::string& message = message_deque_.front();
            auto view = lcy::asio::buffer_view(message.c_str(), message.length());

            lcy::asio::async_write(tcp_socket_, view,
                lcy::asio::strand_wrapper(std::move(on_one_message_send_completed)));
        };

        strand_.post(std::move(do_async_send_func));
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