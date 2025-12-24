#ifndef __TCP_SERVER_HPP__
#define __TCP_SERVER_HPP__

#include <iostream>
#include <functional>

#include <lcy/asio/asio.hpp>

template <class T>		// connection_type
class TCPServer {
public:
	typedef std::function<void (T&)> conn_init_op_type;

	TCPServer(lcy::asio::IOContext& ioc, size_t thread_num);
	~TCPServer();

	void setConnInitOp(conn_init_op_type init_op);
	void start(const lcy::asio::ip::Endpoint& endpoint);

private:
	void start_accept();

private:
	lcy::asio::ip::TCP::Acceptor acceptor_;
	lcy::asio::ThreadPool io_thread_pool_;

	conn_init_op_type conn_init_op_;
};

/////////////////////////////////////////////////////////

template <class T>
TCPServer<T>::TCPServer(lcy::asio::IOContext& ioc, size_t thread_num) :
	acceptor_(ioc),
	io_thread_pool_(thread_num)
{
}

template <class T>
TCPServer<T>::~TCPServer()
{
	acceptor_.cancel();
	io_thread_pool_.stop();
}


template <class T>
void TCPServer<T>::setConnInitOp(conn_init_op_type init_op)
{
	conn_init_op_ = std::move(init_op);
}

template <class T>
void TCPServer<T>::start(const lcy::asio::ip::Endpoint& endpoint)
{
	io_thread_pool_.start();
	acceptor_.setup(endpoint);

	start_accept();
}

template <class T>
void TCPServer<T>::start_accept()
{
	lcy::asio::IOContext& ioc = io_thread_pool_.nextContext();
	auto conn = std::make_shared<T>(ioc);

	acceptor_.async_accept(conn->get_socket(),
			[this, conn, &ioc](lcy::asio::errcode_type ec){
		if ( !ec ) {
			if ( conn_init_op_ ) {
				conn_init_op_(*conn);
			}
			
			lcy::asio::post(ioc, [conn](){
				conn->start();
			});

			// accept again
			start_accept();

		} else {
			if ( ec != lcy::asio::err::EOPCANCELED ) {
				std::cout << "server acceptor accept : "
					  	<< lcy::asio::errinfo(ec) << std::endl;
			}
		}
	});
}

#endif	// __TCP_SERVER_HPP__
