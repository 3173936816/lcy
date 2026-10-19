#include <iostream>

#include "lcy/asio/steady_timer.hpp"

int main()
{
	lcy::asio::io_context ioc;
	lcy::asio::steady_timer timer(ioc);

	timer.expires_after(3000);
	timer.async_wait([](lcy::system::error_code ec){
		if ( ec ) {
			std::cout << "ec : " << ec.message() << std::endl;
		} else {
			std::cout << "timer called!!" << std::endl;
		}
	});

	ioc.run();

	return 0;
}
