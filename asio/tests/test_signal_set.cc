#include "../asio.hpp"

#include <iostream>
#include <signal.h>

int main() {

	lcy::asio::IOContext ioc;
	lcy::asio::SignalSet sigset(ioc, SIGINT);

	sigset.async_wait([&ioc](int errcode, int signum) {
		std::cout << "signum : " << signum << std::endl;
		ioc.quit();
	});

	ioc.loop_wait();

	return 0;
}
