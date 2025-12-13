#include "protocol/protocol.hpp"

#include <iostream>
#include <string>

//lcy::protocol::http::Parser::RetCode

void test_parse_http_request_line() {
	lcy::protocol::http::Parser parser;
	lcy::protocol::http::Request request;
	std::string line = "GET / HTTP/1.1\r\nContent-Length: 16\r\ntext: thml\r\n";
	
	auto retcode = parser.parse(&line[0], line.length(), request);
	if ( retcode == lcy::protocol::http::Parser::RetCode::ERROR ) {
		std::cout << "error" << std::endl;
	} else if ( retcode == lcy::protocol::http::Parser::RetCode::WAITING_DATA ) {
		std::cout << "waiting data" << std::endl;
	} else {
		std::cout << "ready" << std::endl;

		std::cout << "method : " << lcy::protocol::http::MethodToString(request.method()) << std::endl;
		std::cout << "uri : " << request.uri() << std::endl;
		std::cout << "version : " << lcy::protocol::http::VersionToString(request.version()) << std::endl;

		for ( auto& kv : request.getHeadersRef() ) {
			std::cout << kv.first << "--" << kv.second << std::endl; 
		}

		std::cout << "nparse : " << parser.nparse() << std::endl;
	}

	std::cout << std::endl;

	line += "hhhh : kkk \r\n\r\n";
	retcode = parser.parse(&line[0], line.length(), request);
	if ( retcode == lcy::protocol::http::Parser::RetCode::ERROR ) {
		std::cout << "error" << std::endl;
	} else if ( retcode == lcy::protocol::http::Parser::RetCode::WAITING_DATA ) {
		std::cout << "waiting data" << std::endl;
	} else {
		std::cout << "ready" << std::endl;

		std::cout << "method : " << lcy::protocol::http::MethodToString(request.method()) << std::endl;
		std::cout << "uri : " << request.uri() << std::endl;
		std::cout << "version : " << lcy::protocol::http::VersionToString(request.version()) << std::endl;

		for ( auto& kv : request.getHeadersRef() ) {
			std::cout << kv.first << "--" << kv.second << std::endl; 
		}

		std::cout << "nparse : " << parser.nparse() << std::endl;
	}
}

void test_parse_http_response_line() {
	lcy::protocol::http::Parser parser;
	lcy::protocol::http::Response response;
	std::string line = "HTTP/1.1 200 ok\r\n";
	
	auto retcode = parser.parse(&line[0], line.length(), response);
	if ( retcode == lcy::protocol::http::Parser::RetCode::ERROR ) {
		std::cout << "error" << std::endl;
	} else if ( retcode == lcy::protocol::http::Parser::RetCode::WAITING_DATA ) {
		std::cout << "waiting data" << std::endl;
	} else {
		std::cout << "ready" << std::endl;

		std::cout << "version : " << lcy::protocol::http::VersionToString(response.version()) << std::endl;
		std::cout << "state : " << (int)response.state() << std::endl;
		std::cout << "description : " << lcy::protocol::http::StateToDesc(response.state()) << std::endl;
		std::cout << "nparse : " << parser.nparse() << std::endl;
	}
}

int main() {
	 test_parse_http_request_line();
	// test_parse_http_response_line();

	return 0;
}
