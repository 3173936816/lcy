#include <iostream>
#include <signal.h>

#include "lcy/asio/asio.hpp"

void server()
{
	lcy::asio::IOContext ioc;
	lcy::asio::ip::Endpoint endpoint("::", 9950);
	lcy::asio::ip::TCP::Acceptor acceptor(ioc, endpoint);

	lcy::asio::ip::TCP::Socket client_socket(ioc);
	acceptor.async_accept(client_socket, [&client_socket](int errcode){
		if ( !errcode ) {

			lcy::asio::ip::Endpoint endpoint;
			client_socket.peerAddr(endpoint);
			std::cout << "peer : " << endpoint.ip() << " " << endpoint.port() << std::endl;
			
			std::cout << "tcpinfo : \n" << client_socket.tcpInfo() << std::endl; 

			char* buff = new char[6]{ "hello" };
			client_socket.async_write(lcy::asio::buffer(buff, 6), [buff, &client_socket](int, size_t){
				delete[] buff;
				client_socket.shutdown();
			});

		} else {
			std::cout << "acceptor: " << lcy::asio::errinfo(errcode) << std::endl;
		}
	});

	lcy::asio::SignalSet sig(ioc, SIGINT);
	sig.async_wait([&ioc](int errcode, int signal){
		if ( !errcode ) {
			ioc.quit();
		} else {
			std::cout << "sig: " << lcy::asio::errinfo(errcode) << std::endl;
		}
	});

	ioc.loop_wait();
}

int main() {

	server();

	return 0;
}
