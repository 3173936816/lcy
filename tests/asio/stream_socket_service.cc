#include "lcy/asio/ip/tcp.hpp"
#include "lcy/asio/detail/scheduler.hpp"
#include "lcy/asio/detail/stream_socket_service.hpp"

int main()
{
	lcy::system::error_code ec;
	lcy::asio::detail::scheduler sch(1);
	lcy::asio::detail::stream_socket_service service(sch.get_reactor());
	lcy::asio::detail::stream_socket_service::socket_impl server_impl;
	lcy::asio::detail::stream_socket_service::socket_impl client_impl;

	service.construct(server_impl);
	service.construct(client_impl);

	service.open(server_impl, lcy::asio::ip::tcp::v4(), ec);
	if ( ec ) {
		return 1;
	}

	const unsigned short listen_port = 34567;
	lcy::asio::ip::tcp::endpoint bind_endpoint("127.0.0.1", listen_port);
	service.bind(server_impl, bind_endpoint, ec);
	if ( ec ) {
		return 2;
	}

	service.listen(server_impl, 1, ec);
	if ( ec ) {
		return 3;
	}

	service.open(client_impl, lcy::asio::ip::tcp::v4(), ec);
	if ( ec ) {
		return 4;
	}

	service.connect(client_impl, bind_endpoint, ec);
	if ( ec ) {
		service.close(client_impl, ec);
		service.close(server_impl, ec);
		return 5;
	}

	service.close(client_impl, ec);
	service.close(server_impl, ec);
	return 0;
}
