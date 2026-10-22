#include <iostream>

#include "lcy/asio/ip/tcp.hpp"
#include "lcy/asio/ip/udp.hpp"

void test_tcp_resolver()
{
	lcy::asio::ip::tcp::resolver resolver;

	std::list<lcy::asio::ip::tcp::endpoint> endpoints;
	resolver.resolve("www.baidu.com", "http", lcy::asio::ip::tcp::v4(), endpoints);

	for ( auto& endpoint : endpoints ) {
		std::cout << endpoint.to_string() << std::endl;
	}
}

int main()
{
	test_tcp_resolver();

	return 0;
}
