#include <lcy/asio/asio.hpp>
#include <memory>
#include <string>

class TCPConnection : public std::enable_shared_from_this<TCPConnection> {
public:
	TCPConnection(lcy::asio::IOContext& ioc);
	~TCPConnection();

	void start();
	void shutdown();

	lcy::asio::ip::TCP::Socket& get_socket();

private:
	void start_read();
	void processData();

private:
	lcy::asio::ip::TCP::Socket socket_;
	lcy::asio::DynamicBuffer read_buffer_;
	lcy::asio::DynamicBuffer write_buffer_;
};
