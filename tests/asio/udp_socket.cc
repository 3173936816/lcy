// test_udp_echo.cc
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

#include "lcy/asio/io_context.hpp"
#include "lcy/asio/ip/udp.hpp"
#include "lcy/asio/buffer_view.hpp"

using namespace lcy::asio;

static std::atomic<int> g_recv_count{0};
static std::atomic<int> g_send_count{0};

class udp_connection : public std::enable_shared_from_this<udp_connection>
{
public:
    typedef std::shared_ptr<udp_connection> sptr;

    static sptr create_server(io_context& ioc, ip::udp::endpoint local_ep)
    {
        sptr conn(new udp_connection(ioc));
        lcy::system::error_code ec;
        conn->socket_.open(local_ep.protocol(), ec);
        if ( ec ) { std::cout << "  server open: " << ec.message() << "\n"; return nullptr; }
        conn->socket_.bind(local_ep, ec);
        if ( ec ) { std::cout << "  server bind: " << ec.message() << "\n"; return nullptr; }
        return conn;
    }

    static sptr create_client(io_context& ioc, ip::udp proto)
    {
        sptr conn(new udp_connection(ioc));
        lcy::system::error_code ec;
        conn->socket_.open(proto, ec);
        if ( ec ) { std::cout << "  client open: " << ec.message() << "\n"; return nullptr; }
        return conn;
    }

    static sptr create_connect_client(io_context& ioc, ip::udp::endpoint remote_ep)
    {
        sptr conn(new udp_connection(ioc));
        lcy::system::error_code ec;
        conn->socket_.open(remote_ep.protocol(), ec);
        if ( ec ) { std::cout << "  client open: " << ec.message() << "\n"; return nullptr; }
        conn->socket_.connect(remote_ep, ec);
        if ( ec ) { std::cout << "  client connect: " << ec.message() << "\n"; return nullptr; }
        return conn;
    }

    void do_receive_from_read()
    {
        auto self = shared_from_this();
        auto remote = std::make_shared<ip::udp::endpoint>();

        socket_.async_receive_from(*remote, buffer_view(read_buffer_),
            [remote, this, self](lcy::system::error_code ec, std::size_t n) {
                if ( ec ) { std::cout << "  recv_from error: " << ec.message() << "\n"; return; }
                ++g_recv_count;
                std::cout << "  recv_from " << n << " bytes from "
                          << remote->to_string() << "\n";
                std::memcpy(write_buffer_.data(), read_buffer_.data(), n);
                do_send_to_write(*remote, const_buffer_view(write_buffer_.data(), n));
            });
    }

    void do_send_write(const_buffer_view bv)
    {
        auto self = shared_from_this();
        socket_.async_send(bv,
            [this, self](lcy::system::error_code ec, std::size_t n) {
                if ( ec ) { std::cout << "  send error: " << ec.message() << "\n"; return; }
                ++g_send_count;
                std::cout << "  send " << n << " bytes\n";
                do_receive_read();
            });
    }

    void do_receive_read()
    {
        auto self = shared_from_this();
        socket_.async_receive(buffer_view(read_buffer_),
            [this, self](lcy::system::error_code ec, std::size_t n) {
                if ( ec ) { std::cout << "  recv error: " << ec.message() << "\n"; return; }
                ++g_recv_count;
                std::cout << "  recv " << n << " bytes\n";
                std::memcpy(write_buffer_.data(), read_buffer_.data(), n);
                do_send_write(const_buffer_view(write_buffer_.data(), n));
            });
    }

    void do_send_to_loop(ip::udp::endpoint remote)
    {
        auto self = shared_from_this();
        static const char msg[] = "ping";
        socket_.async_send_to(remote, const_buffer_view(msg, sizeof(msg) - 1),
            [this, self, remote](lcy::system::error_code ec, std::size_t n) {
                if ( ec ) { std::cout << "  send_to error: " << ec.message() << "\n"; return; }
                ++g_send_count;
                std::cout << "  send_to " << n << " bytes\n";
                do_recv_from_loop(remote);
            });
    }

    void do_recv_from_loop(ip::udp::endpoint remote)
    {
        (void)remote;

        auto self = shared_from_this();
        auto from = std::make_shared<ip::udp::endpoint>();
        socket_.async_receive_from(*from, buffer_view(read_buffer_),
            [this, self](lcy::system::error_code ec, std::size_t n) {
                if ( ec ) { std::cout << "  recv_from error: " << ec.message() << "\n"; return; }
                ++g_recv_count;
                std::cout << "  recv_from " << n << " bytes\n";
            });
    }

    void close() { socket_.close(); }
    ip::udp::socket& socket() { return socket_; }

private:
    void do_send_to_write(ip::udp::endpoint remote, const_buffer_view bv)
    {
        auto self = shared_from_this();
        socket_.async_send_to(remote, bv,
            [this, self](lcy::system::error_code ec, std::size_t n) {
                if ( ec ) { std::cout << "  send_to error: " << ec.message() << "\n"; return; }
                ++g_send_count;
                std::cout << "  send_to " << n << " bytes\n";
                do_receive_from_read();
            });
    }

    explicit udp_connection(io_context& ioc) : socket_(ioc) {}

    std::array<char, 2048> read_buffer_;
    std::array<char, 2048> write_buffer_;
    ip::udp::socket socket_;
};

struct udp_server_fixture
{
    io_context ioc;
    udp_connection::sptr server;
    std::vector<std::thread> runners;
    ip::udp::endpoint ep;

    udp_server_fixture(std::size_t nthreads, unsigned short port)
        : ioc(nthreads), ep(ip::udp::v4(), port)
    {
        server = udp_connection::create_server(ioc, ep);
        assert(server);
        server->do_receive_from_read();

        for ( std::size_t i = 0; i < nthreads; ++i ) {
            runners.emplace_back([this]() { ioc.run(); });
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    ~udp_server_fixture()
    {
        ioc.stop();
        for ( auto& t : runners ) {
            if ( t.joinable() ) t.join();
        }
    }
};

static bool udp_echo_sync(io_context& ioc, ip::udp::endpoint server_ep,
                          const std::string& msg, std::string& reply)
{
    ip::udp::socket sock(ioc);
    lcy::system::error_code ec;
    sock.open(ip::udp::v4(), ec);
    if ( ec ) return false;

    std::size_t sent = sock.send_to(server_ep,
        const_buffer_view(msg.data(), msg.size()), ec);
    if ( ec || sent != msg.size() ) return false;

    reply.assign(2048, '\0');
    ip::udp::endpoint from;
    std::size_t got = sock.receive_from(from,
        mutable_buffer_view(&reply[0], reply.size()), ec);
    if ( ec ) return false;

    reply.resize(got);
    sock.close();
    return reply == msg;
}

static void test_server_echo()
{
    std::cout << "[test_server_echo]\n";
    g_recv_count = 0;
    g_send_count = 0;

    udp_server_fixture fx(4, 19960);

    std::string reply;
    assert(udp_echo_sync(fx.ioc, fx.ep, "hello!", reply));
    assert(reply == "hello!");

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    assert(g_recv_count >= 1);
    assert(g_send_count >= 1);
    std::cout << "  echo ok\n";
}

static void test_server_multi_echo()
{
    std::cout << "[test_server_multi_echo]\n";
    g_recv_count = 0;
    g_send_count = 0;

    udp_server_fixture fx(4, 19961);

    for ( int i = 0; i < 10; ++i ) {
        std::string msg = "msg" + std::to_string(i);
        std::string reply;
        assert(udp_echo_sync(fx.ioc, fx.ep, msg, reply));
        assert(reply == msg);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    assert(g_recv_count >= 10);
    assert(g_send_count >= 10);
    std::cout << "  10 echoes ok\n";
}

static void test_server_large()
{
    std::cout << "[test_server_large]\n";
    g_recv_count = 0;
    g_send_count = 0;

    udp_server_fixture fx(4, 19962);

    std::string msg(1400, 'x');
    std::string reply;
    assert(udp_echo_sync(fx.ioc, fx.ep, msg, reply));
    assert(reply == msg);
    std::cout << "  1400-byte echo ok\n";
}

static void test_connect_client()
{
    std::cout << "[test_connect_client]\n";
    g_recv_count = 0;
    g_send_count = 0;

    io_context ioc(4);
    ip::udp::endpoint server_ep(ip::udp::v4(), 19963);

    auto server = udp_connection::create_server(ioc, server_ep);
    assert(server);
    server->do_receive_from_read();

    std::vector<std::thread> runners;
    for ( int i = 0; i < 4; ++i ) {
        runners.emplace_back([&ioc]() { ioc.run(); });
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto client = udp_connection::create_connect_client(ioc, server_ep);
    assert(client);

    std::string msg = "hello world";
    client->do_send_write(const_buffer_view(msg.data(), msg.size()));

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    assert(g_send_count >= 1);
    assert(g_recv_count >= 1);
    std::cout << "  connect client echo ok, send=" << g_send_count
              << " recv=" << g_recv_count << "\n";

    ioc.stop();
    for ( auto& t : runners ) t.join();
}

static void test_unconnect_client()
{
    std::cout << "[test_unconnect_client]\n";
    g_recv_count = 0;
    g_send_count = 0;

    io_context ioc(4);
    ip::udp::endpoint server_ep(ip::udp::v4(), 19964);

    auto server = udp_connection::create_server(ioc, server_ep);
    assert(server);
    server->do_receive_from_read();

    std::vector<std::thread> runners;
    for ( int i = 0; i < 4; ++i ) {
        runners.emplace_back([&ioc]() { ioc.run(); });
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto client = udp_connection::create_client(ioc, ip::udp::v4());
    assert(client);
    client->do_send_to_loop(server_ep);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    assert(g_send_count >= 1);
    assert(g_recv_count >= 1);
    std::cout << "  unconnect client ok, send=" << g_send_count
              << " recv=" << g_recv_count << "\n";

    ioc.stop();
    for ( auto& t : runners ) t.join();
}

static void test_bind_conflict()
{
    std::cout << "[test_bind_conflict]\n";
    io_context ioc(1);
    ip::udp::endpoint ep(ip::udp::v4(), 19965);

    auto s1 = udp_connection::create_server(ioc, ep);
    assert(s1);

    auto s2 = udp_connection::create_server(ioc, ep);
    assert(!s2);
    std::cout << "  second bind failed as expected\n";
}

static void test_not_open()
{
    std::cout << "[test_not_open]\n";
    io_context ioc(1);
    auto conn = udp_connection::create_client(ioc, ip::udp::v4());
    assert(conn);
    conn->close();

    std::atomic<bool> got_error{false};
    char buf[8];
    lcy::system::error_code ec;
    ip::udp::endpoint from;
    auto n = conn->socket().receive_from(
        from, mutable_buffer_view(buf, sizeof(buf)), ec);
    assert(ec);
    (void)n;
    std::cout << "  not open error: " << ec.message() << "\n";
}

int main()
{
    test_server_echo();
    test_server_multi_echo();
    test_server_large();
    test_connect_client();
    test_unconnect_client();
    test_bind_conflict();
    test_not_open();

    std::cout << "\nall udp echo tests passed\n";
    return 0;
}