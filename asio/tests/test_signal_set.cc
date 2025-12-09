#include "../asio.hpp"

#include <iostream>
#include <signal.h>

int main() {

	lcy::asio::IOContext ioc;
	lcy::asio::SignalSet sigset(ioc, SIGINT);

	sigset.async_wait([&ioc](int errcode, int signum) {
		if ( !errcode ) {
			std::cout << "signum : " << signum << std::endl;
			ioc.quit();
		} else {
			std::cout << "1 " << lcy::asio::errinfo(errcode) << std::endl;
		}
	});

	sigset.async_wait([&ioc](int errcode, int signum) {
		if ( !errcode ) {
			std::cout << "signum : " << signum << std::endl;
			ioc.quit();
		} else {
			std::cout << "2 " << lcy::asio::errinfo(errcode) << std::endl;
		}
	});

	ioc.loop_wait();

	return 0;
}
