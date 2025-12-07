#include "asio/asio.hpp"
#include <iostream>
#include <signal.h>

void server()
{
	lcy::asio::IOContext ioc;
	lcy::asio::ip::UDP::Socket udp_socket(ioc);
	lcy::asio::ip::EndPoint endpoint("::", 9950);

	udp_socket.open(lcy::asio::ip::UDP::v6());
	udp_socket.setReuseAddr();
	udp_socket.bind(endpoint);

	lcy::asio::ip::EndPoint client_addr;
	char buff[1024] = { 0 };

//	std::cout << "buff : " << (intptr_t)buff << std::endl;
//	std::cout << "tmp : " << (intptr_t)lcy::asio::buffer(buff, 1024).data() << std::endl;

	udp_socket.async_read(client_addr, lcy::asio::buffer(buff, 1024), [&buff, &client_addr, &udp_socket](int errcode, size_t nbytes) {
		if ( !errcode ) {

			std::cout << "recv = " << buff << ", nbytes = " << nbytes << std::endl;

			udp_socket.async_write(client_addr, lcy::asio::buffer(buff, nbytes), [](int, size_t nbytes){
				std::cout << "send ok" << std::endl;
			});

		} else {
			std::cout << lcy::asio::errinfo(errcode) << std::endl;
		}
	});

	lcy::asio::SignalSet sig(ioc, SIGINT);
	sig.async_wait([&ioc](int errcode, int signal){
		ioc.quit();
	});

	ioc.loop_wait();
}

int main() {

	server();

	return 0;
}
