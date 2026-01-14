#include <lcy/asio/asio.hpp>
#include <lcy/protocol/protocol.hpp>

#include <deque>
#include <memory>

typedef
std::function<void (
	const lcy::protocol::http::Request&,
	lcy::protocol::http::Response&
)> http_op_type;


class HttpConnection : 
	public std::enable_shared_from_this<HttpConnection>
{
public:
	HttpConnection(lcy::asio::IOContext& ioc);
	~HttpConnection();

	void start();

	void setHttpOperation(http_op_type http_op);
	lcy::asio::ip::TCP::Socket& get_socket();

private:
	void start_read();
	void start_send(std::string msg, bool shutdown);
	void start_send_impl();

private:
	typedef std::pair<std::string, bool> response_op_type;
	typedef std::deque<response_op_type> response_op_deque_type;

	lcy::asio::ip::TCP::Socket socket_;
	lcy::asio::DynamicBuffer buffer_;
	lcy::protocol::http::Parser parser_;
	lcy::protocol::http::Request request_;
	response_op_deque_type response_op_deque_;

	http_op_type http_op_;
};
