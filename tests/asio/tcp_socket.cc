// test_echo_server_all.cc
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>

#include "lcy/asio/io_context.hpp"
#include "lcy/asio/ip/tcp.hpp"
#include "lcy/asio/buffer_view.hpp"
#include "lcy/asio/signal_set.hpp"
#include "lcy/asio/error.hpp"
#include "lcy/asio/detail/error.hpp"

using namespace lcy::asio;

class tcp_connection : public std::enable_shared_from_this<tcp_connection>
{
public:
    typedef std::shared_ptr<tcp_connection> sptr;

    explicit tcp_connection(io_context& ioc) : socket_(ioc) {}

    void do_read()
    {
        auto self = shared_from_this();
        socket_.async_read_some(buffer_view(read_buffer_),
            [this, self](lcy::system::error_code ec, std::size_t n) {
                if ( ec ) {
                    std::cout << "    [conn] read error: " << ec.message() << "\n";
                    return;
                }
                std::cout << "    [conn] read " << n << " bytes\n";
                std::memcpy(write_buffer_.data(), read_buffer_.data(), n);
                do_write(const_buffer_view(write_buffer_.data(), n));
            });
    }

    void do_write(const_buffer_view bv)
    {
        auto self = shared_from_this();
        socket_.async_write_some(bv,
            [this, self](lcy::system::error_code ec, std::size_t n) {
                if ( ec ) {
                    std::cout << "    [conn] write error: " << ec.message() << "\n";
                    return;
                }
                std::cout << "    [conn] wrote " << n << " bytes\n";
                ++g_echo_count;
                do_read();
            });
    }

    void cancel() { socket_.cancel(); }
    void close()  { socket_.close(); }
    ip::tcp::socket& socket() { return socket_; }

    static std::atomic<int> g_echo_count;

private:
    std::array<char, 2048> read_buffer_;
    std::array<char, 2048> write_buffer_;
    ip::tcp::socket socket_;
};
std::atomic<int> tcp_connection::g_echo_count{0};

class tcp_server
{
public:
    tcp_server(io_context& ioc, ip::tcp::endpoint ep)
        : ioc_(ioc), acceptor_(ioc, ep) {}

    void do_accept()
    {
        auto conn = std::make_shared<tcp_connection>(ioc_);
        auto remote = std::make_shared<ip::tcp::endpoint>();

        acceptor_.async_accept(conn->socket(), *remote,
            [conn, remote, this](lcy::system::error_code ec) {
                if ( ec ) {
                    std::cout << "    [server] accept error: " << ec.message() << "\n";
                    return;
                }
                std::cout << "    [server] accepted " << remote->to_string() << "\n";
                conn->do_read();
                do_accept();
            });
    }

    ip::tcp::endpoint local_endpoint()
    {
        ip::tcp::endpoint ep;
        acceptor_.local_endpoint(ep);
        return ep;
    }

private:
    io_context& ioc_;
    ip::tcp::acceptor acceptor_;
};

struct echo_client
{
    io_context& ioc;
    ip::tcp::socket sock;

    explicit echo_client(io_context& i)
        : ioc(i), sock(i) {}

    bool connect(ip::tcp::endpoint ep)
    {
        lcy::system::error_code ec;
        sock.open(ip::tcp::v4(), ec);
        if ( ec ) return false;
        sock.connect(ep, ec);
        return !ec;
    }

    bool send_msg(const std::string& msg)
    {
        lcy::system::error_code ec;
        std::size_t n = sock.send(const_buffer_view(msg.data(), msg.size()), ec);
        return !ec && n == msg.size();
    }

    bool recv_exact(std::string& out, std::size_t len)
    {
        out.assign(len, '\0');
        std::size_t got = 0;
        lcy::system::error_code ec;
        while ( got < len ) {
            std::size_t n = sock.receive(
                mutable_buffer_view(&out[got], len - got), ec);
            if ( ec ) return false;
            if ( n == 0 ) return false;
            got += n;
        }
        return true;
    }

    bool echo(const std::string& msg)
    {
        if ( !send_msg(msg) ) return false;
        std::string reply;
        if ( !recv_exact(reply, msg.size()) ) return false;
        return reply == msg;
    }

    void close() { sock.close(); }
};

struct server_fixture
{
    io_context ioc;
    tcp_server server;
    std::vector<std::thread> runners;
    ip::tcp::endpoint ep;

    server_fixture(std::size_t run_threads, unsigned short port)
        : ioc(run_threads),
          server(ioc, ip::tcp::endpoint(ip::tcp::v4(), port)),
          ep(ip::tcp::v4(), port)
    {
        server.do_accept();
        for ( std::size_t i = 0; i < run_threads; ++i ) {
            runners.emplace_back([this]() { ioc.run(); });
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    ~server_fixture()
    {
        ioc.stop();
        for ( auto& t : runners ) {
            if ( t.joinable() ) t.join();
        }
    }
};

static void test_basic_echo()
{
    std::cout << "[test_basic_echo]\n";
    tcp_connection::g_echo_count = 0;

    server_fixture fx(4, 19950);

    echo_client client(fx.ioc);
    assert(client.connect(fx.ep));
    assert(client.echo("hello!"));
    std::cout << "  echo ok\n";

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    assert(tcp_connection::g_echo_count == 1);
    std::cout << "  echo_count=1\n";
}

static void test_multi_echo()
{
    std::cout << "[test_multi_echo]\n";
    tcp_connection::g_echo_count = 0;

    server_fixture fx(4, 19951);

    echo_client client(fx.ioc);
    assert(client.connect(fx.ep));

    for ( int i = 0; i < 10; ++i ) {
        std::string msg = "msg" + std::to_string(i);
        assert(client.echo(msg));
    }
    std::cout << "  10 echoes ok\n";

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    assert(tcp_connection::g_echo_count == 10);
    std::cout << "  echo_count=10\n";
}

static void test_multi_connections()
{
    std::cout << "[test_multi_connections]\n";
    tcp_connection::g_echo_count = 0;

    server_fixture fx(8, 19952);

    const int N = 8;
    std::vector<std::thread> clients;
    std::atomic<int> ok_count{0};

    for ( int i = 0; i < N; ++i ) {
        clients.emplace_back([&, i]() {
            echo_client c(fx.ioc);
            if ( !c.connect(fx.ep) ) return;
            std::string msg = "conn" + std::to_string(i);
            if ( c.echo(msg) ) ++ok_count;
            c.close();
        });
    }
    for ( auto& t : clients ) t.join();

    assert(ok_count == N);
    std::cout << "  " << ok_count << "/" << N << " connections ok\n";

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    assert(tcp_connection::g_echo_count == N);
}

static void test_large_message()
{
    std::cout << "[test_large_message]\n";
    tcp_connection::g_echo_count = 0;

    server_fixture fx(4, 19953);

    echo_client client(fx.ioc);
    assert(client.connect(fx.ep));

    std::string msg(2000, 'x');
    assert(client.echo(msg));
    std::cout << "  2000-byte echo ok\n";

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    assert(tcp_connection::g_echo_count == 1);
}

static void test_client_close()
{
    std::cout << "[test_client_close]\n";
    tcp_connection::g_echo_count = 0;

    server_fixture fx(4, 19954);

    {
        echo_client client(fx.ioc);
        assert(client.connect(fx.ep));
        assert(client.echo("bye"));
        client.close();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "  client close handled\n";
}

static void test_client_not_connected()
{
    std::cout << "[test_client_not_connected]\n";
    server_fixture fx(2, 19955);

    echo_client client(fx.ioc);
    lcy::system::error_code ec;
    client.sock.open(ip::tcp::v4(), ec);
    assert(!ec);
    std::size_t n = client.sock.send(
        const_buffer_view("x", 1), ec);
    (void)n;
    assert(ec);
    std::cout << "  send without connect failed as expected: "
              << ec.message() << "\n";
    client.close();
}

static void test_sigint_exit()
{
    std::cout << "[test_sigint_exit]\n";

    io_context ioc(4);
    signal_set sig(ioc, { SIGINT });
    std::atomic<bool> sigint_fired{false};
    sig.async_wait([&](lcy::system::error_code ec, int) {
        if ( !ec ) {
            sigint_fired = true;
            ioc.stop();
        }
    });

    ip::tcp::endpoint ep(ip::tcp::v4(), 19956);
    tcp_server server(ioc, ep);
    server.do_accept();

    std::vector<std::thread> runners;
    for ( int i = 0; i < 4; ++i ) {
        runners.emplace_back([&ioc]() { ioc.run(); });
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    ::kill(::getpid(), SIGINT);

    for ( auto& t : runners ) {
        if ( t.joinable() ) t.join();
    }

    assert(sigint_fired);
    std::cout << "  SIGINT exit ok\n";
}

static unsigned short unused_tcp_port()
{
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    assert(fd != -1);

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    assert(::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0);

    socklen_t len = sizeof(addr);
    assert(::getsockname(fd, reinterpret_cast<sockaddr*>(&addr), &len) == 0);
    unsigned short port = ntohs(addr.sin_port);
    ::close(fd);
    return port;
}

class full_backlog_listener
{
public:
    full_backlog_listener() : fd_(-1), port_(0)
    {
        fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        assert(fd_ != -1);

        int one = 1;
        ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

        sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = 0;
        assert(::bind(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0);

        socklen_t len = sizeof(addr);
        assert(::getsockname(fd_, reinterpret_cast<sockaddr*>(&addr), &len) == 0);
        port_ = ntohs(addr.sin_port);

        assert(::listen(fd_, 1) == 0);

        addr.sin_port = htons(port_);
        for ( int i = 0; i < 8; ++i ) {
            int c = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
            assert(c != -1);
            ::connect(c, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
            fillers_.push_back(c);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    ~full_backlog_listener()
    {
        for ( int fd : fillers_ ) {
            ::close(fd);
        }
        if ( fd_ != -1 ) {
            ::close(fd_);
        }
    }

    bool queue_is_full() const
    {
        for ( int fd : fillers_ ) {
            pollfd p;
            p.fd = fd;
            p.events = POLLOUT;
            p.revents = 0;
            if ( ::poll(&p, 1, 0) == 0 ) {
                return true;
            }
        }
        return false;
    }

    unsigned short port() const { return port_; }

private:
    int fd_;
    unsigned short port_;
    std::vector<int> fillers_;
};

static void test_async_connect_refused()
{
    std::cout << "[test_async_connect_refused]\n";

    io_context ioc(1);
    ip::tcp::socket sock(ioc);
    lcy::system::error_code ec;
    sock.open(ip::tcp::v4(), ec);
    assert(!ec);

    unsigned short port = unused_tcp_port();

    bool done = false;
    lcy::system::error_code result;
    sock.async_connect(ip::tcp::endpoint("127.0.0.1", port),
        [&](lcy::system::error_code e) {
            done = true;
            result = e;
        });
    ioc.run();

    assert(done);
    assert(result == lcy::system::error_code(
        lcy::system::error::connection_refused));
    std::cout << "  connect reported: " << result.message() << "\n";
    sock.close();
}

static void test_async_connect_cancel()
{
    std::cout << "[test_async_connect_cancel]\n";

    full_backlog_listener listener;
    if ( !listener.queue_is_full() ) {
        std::cout << "  skipped: kernel completed the connect immediately\n";
        return;
    }

    io_context ioc(1);
    ip::tcp::socket sock(ioc);
    lcy::system::error_code ec;
    sock.open(ip::tcp::v4(), ec);
    assert(!ec);

    bool done = false;
    lcy::system::error_code result;
    sock.async_connect(ip::tcp::endpoint("127.0.0.1", listener.port()),
        [&](lcy::system::error_code e) {
            done = true;
            result = e;
        });

    sock.cancel();
    ioc.run();

    assert(done);
    assert(result == lcy::asio::detail::make_asio_error_code(
        lcy::asio::error::operation_abort));
    std::cout << "  cancelled connect reported: " << result.message() << "\n";
    sock.close();
}

static void test_async_receive_after_rst()
{
    std::cout << "[test_async_receive_after_rst]\n";

    io_context ioc(1);
    ip::tcp::acceptor acceptor(ioc);
    lcy::system::error_code ec;
    acceptor.open(ip::tcp::v4(), ec);
    assert(!ec);
    acceptor.bind(ip::tcp::endpoint(ip::tcp::v4(), 0), ec);
    assert(!ec);
    acceptor.listen(1, ec);
    assert(!ec);

    ip::tcp::endpoint ep;
    acceptor.local_endpoint(ep, ec);
    assert(!ec);

    ip::tcp::socket client(ioc);
    client.open(ip::tcp::v4(), ec);
    assert(!ec);
    client.connect(ep, ec);
    assert(!ec);

    ip::tcp::socket peer(ioc);
    ip::tcp::endpoint remote;
    acceptor.accept(peer, remote, ec);
    assert(!ec);

    std::array<char, 64> buffer;
    bool done = false;
    lcy::system::error_code result;
    std::size_t bytes = 0;
    client.async_receive(buffer_view(buffer),
        [&](lcy::system::error_code e, std::size_t n) {
            done = true;
            result = e;
            bytes = n;
        });

    linger lg;
    lg.l_onoff = 1;
    lg.l_linger = 0;
    ::setsockopt(peer.native_handle(), SOL_SOCKET, SO_LINGER, &lg, sizeof(lg));
    peer.close();

    ioc.run();

    assert(done);
    assert(bytes == 0);
    assert(result == lcy::system::error_code(
        lcy::system::error::connection_reset));
    std::cout << "  receive reported: " << result.message() << "\n";
    client.close();
}

int main()
{
    test_basic_echo();
    test_multi_echo();
    test_multi_connections();
    test_large_message();
    test_client_close();
    test_client_not_connected();
    test_async_connect_refused();
    test_async_connect_cancel();
    test_async_receive_after_rst();
    test_sigint_exit();

    std::cout << "\nall echo server tests passed\n";
    return 0;
}