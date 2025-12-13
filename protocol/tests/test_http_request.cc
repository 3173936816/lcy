#include "protocol/protocol.hpp"
#include <iostream>

int main() {

	lcy::protocol::http::Request request;
	request.setVersion(lcy::protocol::http::version::HTTP_1_1);
	request.setMethod(lcy::protocol::http::method::GET);
	request.setUri("/");
	
	request.setHeader("Host", "example.com");
    request.setHeader("User-Agent", "MyHttpClient/1.0");
    request.setHeader("Accept", "application/json");

	std::cout << request.dump() << std::endl;

	return 0;
}
