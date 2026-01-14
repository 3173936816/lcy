#include "lcy/asio/asio.hpp"

#include <signal.h>
#include <iostream>

void client()
{
	lcy::asio::IOContext ioc;
	lcy::asio::ip::TCP::Socket tcp_socket(ioc);
	lcy::asio::ip::Endpoint endpoint("36.152.44.93", 80);

	tcp_socket.open(lcy::asio::ip::TCP::v4());

	tcp_socket.async_connect(endpoint, [&tcp_socket](int errcode) {
		if ( !errcode ) {
			std::cout << "connect successful!" << std::endl;

			std::string* header = new std::string("HEADER / HTTP/1.1\r\n\r\n");

			tcp_socket.async_write(lcy::asio::buffer(*header), [header](int errcode, size_t bytes){
				if ( !errcode ) {
					std::cout << "send header ok : " << bytes << std::endl;
					delete header;
				} else {
					std::cout << 2 << " " << lcy::asio::errinfo(errcode) << std::endl;
				}
			});			

			char* buff = new char[4096]{0};
				
			tcp_socket.async_read(lcy::asio::buffer(buff, 4096), [buff, &tcp_socket](int errcode, size_t bytes){
				if ( !errcode ) {
					std::cout << "recv bytes :  " << bytes << std::endl;
					std::cout << "recv context :\n" << buff << std::endl;
				
					delete[] buff;
					tcp_socket.shutdown();
				} else {
					std::cout << 3 << " " << lcy::asio::errinfo(errcode) << std::endl;
				}
			});			
			
		} else {
			std::cout << 1 << " " << lcy::asio::errinfo(errcode) << std::endl;
		}
	});

	
	lcy::asio::SignalSet sig(ioc, SIGINT);
	sig.async_wait([&ioc](int errcode, int signal){
		ioc.quit();
	}); 

	ioc.loop_wait();
}

void server()
{
	lcy::asio::IOContext ioc;
	lcy::asio::ip::TCP::Socket tcp_socket(ioc);
	lcy::asio::ip::Endpoint endpoint("::", 9950);

	tcp_socket.open(lcy::asio::ip::TCP::v6());
	tcp_socket.setReuseAddr();
	tcp_socket.bind(endpoint);
	tcp_socket.listen(1024);

	lcy::asio::ip::TCP::Socket client_socket(ioc);
	tcp_socket.async_accept(client_socket, [&client_socket](int errcode){
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

	client();
	server();

	return 0;
}
