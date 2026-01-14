#include <lcy/asio/asio.hpp>
#include "../../tcp_server.hpp"
#include "http_connection.h"

#include <iostream>
#include <signal.h>

typedef TCPServer<HttpConnection> HttpServer;


void http_op(const lcy::protocol::http::Request& request,
			 lcy::protocol::http::Response& response)
{
//	std::cout << "request:\n" << request.dump() << std::endl;
	
	response.setVersion(request.version());	
	response.setState(lcy::protocol::http::state::OK);
}

void initOp(HttpConnection& conn)
{
	conn.setHttpOperation(http_op);
}

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

	HttpServer server(ioc, 6);
	
	server.setConnInitOp(initOp);
	server.start(endpoint);

	ioc.loop_wait();

	return 0;
}
