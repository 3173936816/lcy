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

#include "lcy/asio/io_context.hpp"
#include "lcy/asio/ip/tcp.hpp"
#include "lcy/asio/buffer_view.hpp"
#include "lcy/asio/signal_set.hpp"

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

int main()
{
    test_basic_echo();
    test_multi_echo();
    test_multi_connections();
    test_large_message();
    test_client_close();
    test_client_not_connected();
    test_sigint_exit();

    std::cout << "\nall echo server tests passed\n";
    return 0;
}