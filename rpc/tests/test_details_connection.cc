#include "../src/details/connection.h"
#include "../src/details/server.h"
#include "../src/rpc.pb.h"

#include "lcy/asio/asio.hpp"

#include <iostream>

std::shared_ptr<lcy::rpc::details::Connection> conn;

void connectOp(lcy::asio::errcode_type ec)
{
	if ( !ec ) {
		std::cout << "connect success" << std::endl;

		lcy::rpc::Message msg;
		msg.set_id(0);
		msg.set_tp(lcy::rpc::type::request);
		msg.mutable_request()->set_service_name("rpc test name");
		msg.mutable_request()->set_service_method("rpc test method");
		msg.mutable_request()->set_arguments("rpc argument");

		std::string data = msg.SerializeAsString();
		uint32_t len = data.length();
		len = htonl(len);

		data = std::string((char*)&len, sizeof(uint32_t)) + data;

		conn->send(data);
		conn->send(data);
	} else {
		std::cout << lcy::asio::errinfo(ec) << std::endl;
	}
}

void messageOp(lcy::rpc::details::Connection& conn, const std::string& data)
{
	std::cout << data << std::endl;
}

int main() {
	lcy::asio::IOContext ioc;

	conn = std::make_shared<lcy::rpc::details::Connection>(ioc);
	conn->setConnectOp(connectOp);
	conn->setMessageOp(messageOp);

	conn->connect("192.168.21.146", 9950);

	ioc.loop_wait();
	
	return 0;
}
