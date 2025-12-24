#include "../../tcp_server.hpp"
#include "tcp_connection.h"

#include <iostream>
#include <signal.h>

int main() {
	lcy::asio::IOContext ioc;
	lcy::asio::SignalSet sigset(ioc, SIGINT);
	lcy::asio::ip::Endpoint endpoint("0.0.0.0", 9950);

	sigset.async_wait([&ioc](lcy::asio::errcode_type ec, int signum){
		if ( !ec ) {
			ioc.quit();
		} else {
			std::cout << "sigset async_wait : " 
					  << lcy::asio::errinfo(ec) << std::endl;
		}
	});

	std::cout << "server listen in " 
			  << endpoint.ip() 
			  << ":" 
			  << endpoint.port() 
			  << std::endl;
	
	TCPServer<TCPConnection> server(ioc, 5);
	server.start(endpoint);

	ioc.loop_wait();

	return 0;
}
