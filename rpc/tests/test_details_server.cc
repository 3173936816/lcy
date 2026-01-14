#include "../src/details/connection.h"
#include "../src/details/server.h"
#include "../src/rpc.pb.h"

#include "lcy/asio/asio.hpp"

#include <iostream>

void connectionMessageOp(lcy::rpc::details::Connection& conn, const std::string& data)
{
	std::cout << "Connection Message : " << data << std::endl;

	lcy::rpc::Message msg;
	if ( msg.ParseFromString(data) ) {
		std::cout << msg.id() << std::endl;
		std::cout << msg.tp() << std::endl;
		std::cout << msg.mutable_request()->service_name() << std::endl;
		std::cout << msg.mutable_request()->service_method() << std::endl;
		std::cout << msg.mutable_request()->arguments() << std::endl;
	} else {
		std::cout << "parser error" << std::endl;
	}
}

void connectOp(lcy::rpc::details::Connection& conn)
{
	conn.setMessageOp(connectionMessageOp);
}

int main() {
	lcy::asio::IOContext ioc;
	lcy::asio::ip::Endpoint endpoint("0.0.0.0", 9950);

	lcy::rpc::details::Server server(ioc, 1);
	server.setConnectOp(connectOp);

	server.start(endpoint);

	ioc.loop_wait();
	
	return 0;
}
