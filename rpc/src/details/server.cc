#include "lcy/rpc/src/details/server.h"
#include "lcy/rpc/src/details/connection.h"

#include <thread>

#include <iostream>

namespace lcy {
namespace rpc {
namespace details {

Server::Server(lcy::asio::IOContext& ioc, uint32_t threadCount) :
	acceptor_(ioc),
	io_thread_pool_(threadCount)
{
}

Server::~Server()
{
	stop();
}

void Server::start(const lcy::asio::ip::Endpoint& addr)
{
	acceptor_.setup(addr);

	io_thread_pool_.start();
	start_accept();
}

void Server::stop()
{
	acceptor_.cancel();
	io_thread_pool_.stop();
}

void Server::setConnectOp(connect_op_type connect_op)
{
	connect_op_ = std::move(connect_op);
}

void Server::start_accept()
{
	lcy::asio::IOContext& ioc = io_thread_pool_.nextContext();
	auto connection = std::make_shared<Connection>(ioc);

	acceptor_.async_accept(connection->get_socket(),
			[&ioc, connection, this](lcy::asio::errcode_type ec){
		if ( !ec ) {
			if ( connect_op_ ) {
				connect_op_(*connection);
			}

			lcy::asio::post(ioc, [connection](){
				connection->start_recv();
			});

			start_accept();
		}
	});
}

}	// namespace details
}	// namespace rpc
}	// namespace lcy
