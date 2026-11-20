#include <iostream>

#include "lcy/asio/asio.hpp"
#include "lcy/log4cc/log4cc.hpp"

int main()
{
    lcy::asio::io_context ioc;
    bool is_stop = false;

    lcy::asio::signal_set sigset(ioc, { SIGINT });
    sigset.async_wait([&ioc, &is_stop](lcy::system::error_code, int) {
        std::cout << "recv exit signal, exit now" << std::endl;
        is_stop = true;
        ioc.stop();
    });

    lcy::asio::ip::tcp::endpoint endpoint("0.0.0.0", 9950);
    auto tcp_sink = std::make_shared<lcy::log4cc::tcp_sink_mt>(ioc, endpoint, 2);

    auto error_func = [tcp_sink](lcy::system::error_code ec) {
        if ( ec == lcy::system::error::no_buffer_space ) {
            std::cout << "no_buffer" << std::endl;
        } else {
            std::cout << "ec = " << ec.message() << std::endl;
            tcp_sink->reload();
            sleep(1);
        }
    };

    tcp_sink->set_error_func(error_func);

    lcy::log4cc::logger logger("root");
    logger.add_sink(tcp_sink);

    auto log_thread_func = [&logger, &is_stop]() {
        for ( int i = 0; i < 100 && !is_stop; ++i ) {
            sleep(2);

            auto record = std::make_shared<lcy::log4cc::record>(lcy::log4cc::level::debug,
                __FILE__, __LINE__, logger.get_name());
            record->get_oss() << "test message";

            std::cout << "send log" << std::endl;

            logger.log(record);
        }
    };

    std::thread log_thread = std::thread(log_thread_func);

    ioc.run();

    log_thread.join();

    return 0;
}