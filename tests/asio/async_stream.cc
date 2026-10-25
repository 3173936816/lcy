#include <iostream>

#include "lcy/asio/ip/tcp.hpp"
#include "lcy/asio/buffer_view.hpp"
#include "lcy/asio/async_stream.hpp"

int main()
{
    lcy::asio::io_context ioc;

    lcy::asio::ip::tcp::socket socket(ioc);
    socket.open(lcy::asio::ip::tcp::v4());
    socket.connect(lcy::asio::ip::tcp::endpoint("127.0.0.1", 9950));

    char buffer[6];
    lcy::asio::async_read(socket, lcy::asio::mutable_buffer_view(buffer, sizeof(buffer)),
        [](const lcy::system::error_code& ec, std::size_t bytes_transferred) {
            if ( ec ) {
                std::cerr << "Read error: " << ec.message() << std::endl;
                return;
            }
            std::cout << "Read " << bytes_transferred << " bytes." << std::endl;
        });

    ioc.run();

    return 0;
}