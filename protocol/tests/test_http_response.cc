#include "protocol/protocol.hpp"
#include <iostream>

int main() {

	lcy::protocol::http::Response response;
	response.setVersion(lcy::protocol::http::version::HTTP_1_1);
	response.setState(lcy::protocol::http::state::OK);
	
	response.setHeader("Host", "example.com");
    response.setHeader("User-Agent", "MyHttpClient/1.0");
    response.setHeader("Accept", "application/json");
    response.setHeader("Content-Length", "12");

    response.setBody("hello, world");

	std::cout << response.dump() << std::endl;

	return 0;
}
