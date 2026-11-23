// test_stream_io.cc
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "lcy/asio/io_context.hpp"
#include "lcy/asio/ip/tcp.hpp"
#include "lcy/asio/work_guard.hpp"
#include "lcy/asio/buffer_view.hpp"
#include "lcy/asio/stream_read.hpp"
#include "lcy/asio/stream_write.hpp"

using namespace lcy::asio;

struct tcp_pair
{
    io_context ioc;
    work_guard guard;
    ip::tcp::acceptor acceptor;
    ip::tcp::socket client;
    ip::tcp::socket server_conn;
    std::vector<std::thread> runners;

    tcp_pair(unsigned short port, std::size_t nthreads = 2)
        : ioc(nthreads),
          guard(ioc),
          acceptor(ioc, ip::tcp::endpoint(ip::tcp::v4(), port)),
          client(ioc),
          server_conn(ioc)
    {
        lcy::system::error_code ec;
        client.open(ip::tcp::v4());
        client.connect(ip::tcp::endpoint("127.0.0.1", port), ec);
        assert(!ec);

        ip::tcp::endpoint peer;
        acceptor.accept(server_conn, peer, ec);
        assert(!ec);
    }

    void start_runners()
    {
        for ( std::size_t i = 0; i < 2; ++i ) {
            runners.emplace_back([this]() { ioc.run(); });
        }
    }

    ~tcp_pair()
    {
        ioc.stop();
        for ( auto& t : runners ) {
            if ( t.joinable() ) t.join();
        }
    }
};

static void test_sync_read_full()
{
    std::cout << "[test_sync_read_full]\n";
    tcp_pair fx(21001);

    std::thread writer([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        lcy::system::error_code ec;
        fx.server_conn.send(const_buffer_view("hello", 5), ec);
    });

    std::array<char, 5> buf;
    lcy::system::error_code ec;
    std::size_t n = read(fx.client, buffer_view(buf), ec);
    writer.join();

    assert(!ec);
    assert(n == 5);
    assert(std::string(buf.data(), n) == "hello");
    std::cout << "  sync read 5 bytes ok\n";
}

static void test_sync_read_partial_then_full()
{
    std::cout << "[test_sync_read_partial_then_full]\n";
    tcp_pair fx(21002);

    std::thread writer([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        lcy::system::error_code ec;
        fx.server_conn.send(const_buffer_view("abc", 3), ec);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        fx.server_conn.send(const_buffer_view("def", 3), ec);
    });

    std::array<char, 6> buf;
    lcy::system::error_code ec;
    std::size_t n = read(fx.client, buffer_view(buf), ec);
    writer.join();

    assert(!ec);
    assert(n == 6);
    assert(std::string(buf.data(), n) == "abcdef");
    std::cout << "  sync read 6 bytes in two chunks ok\n";
}

static void test_sync_read_eof()
{
    std::cout << "[test_sync_read_eof]\n";
    tcp_pair fx(21003);

    std::thread writer([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        lcy::system::error_code ec;
        fx.server_conn.send(const_buffer_view("ab", 2), ec);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        fx.server_conn.close(ec);
    });

    std::array<char, 6> buf;
    lcy::system::error_code ec;
    std::size_t n = read(fx.client, buffer_view(buf), ec);
    writer.join();

    assert(ec);
    assert(n == 2);
    assert(std::string(buf.data(), n) == "ab");
    std::cout << "  sync read eof after 2 bytes, ec=" << ec.message() << "\n";
}

static void test_sync_read_exception()
{
    std::cout << "[test_sync_read_exception]\n";
    tcp_pair fx(21004);

    std::thread writer([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        lcy::system::error_code ec;
        fx.server_conn.close(ec);
    });

    std::array<char, 6> buf;
    bool threw = false;
    try {
        read(fx.client, buffer_view(buf));
    } catch ( const std::exception& ) {
        threw = true;
    }
    writer.join();

    assert(threw);
    std::cout << "  sync read throws on eof ok\n";
}

static void test_sync_write_full()
{
    std::cout << "[test_sync_write_full]\n";
    tcp_pair fx(21005);

    std::string msg(2000, 'z');
    lcy::system::error_code ec;
    std::size_t n = write(fx.client, const_buffer_view(msg.data(), msg.size()), ec);
    assert(!ec);
    assert(n == msg.size());

    std::string recv(2000, '\0');
    std::size_t got = 0;
    while ( got < 2000 ) {
        lcy::system::error_code ec2;
        std::size_t r = fx.server_conn.receive(
            mutable_buffer_view(&recv[got], 2000 - got), ec2);
        if ( ec2 ) break;
        got += r;
    }

    assert(got == 2000);
    assert(recv == msg);
    std::cout << "  sync write 2000 bytes ok\n";
}

static void test_sync_write_small()
{
    std::cout << "[test_sync_write_small]\n";
    tcp_pair fx(21006);

    lcy::system::error_code ec;
    std::size_t n = write(fx.client, const_buffer_view("abc", 3), ec);
    assert(!ec);
    assert(n == 3);

    std::array<char, 3> buf;
    std::size_t got = 0;
    while ( got < 3 ) {
        lcy::system::error_code ec2;
        std::size_t r = fx.server_conn.receive(
            mutable_buffer_view(&buf[got], 3 - got), ec2);
        if ( ec2 ) break;
        got += r;
    }

    assert(got == 3);
    assert(std::string(buf.data(), 3) == "abc");
    std::cout << "  sync write 3 bytes ok\n";
}

static void test_async_read_full()
{
    std::cout << "[test_async_read_full]\n";
    tcp_pair fx(21007);

    std::thread writer([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        lcy::system::error_code ec;
        fx.server_conn.send(const_buffer_view("hello", 5), ec);
    });

    std::array<char, 5> buf;
    std::atomic<int> calls{0};
    std::string got;
    std::atomic<bool> done{false};

    async_read(fx.client, buffer_view(buf),
        [&](lcy::system::error_code ec, std::size_t n) {
            ++calls;
            if ( !ec ) got.assign(buf.data(), n);
            done = true;
            fx.ioc.stop();
        });

    fx.start_runners();
    writer.join();

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while ( !done && std::chrono::steady_clock::now() < deadline ) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    assert(done);
    assert(calls == 1);
    assert(got == "hello");
    std::cout << "  async read 5 bytes in 1 callback ok\n";
}

static void test_async_read_partial_then_full()
{
    std::cout << "[test_async_read_partial_then_full]\n";
    tcp_pair fx(21008);

    std::thread writer([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        lcy::system::error_code ec;
        fx.server_conn.send(const_buffer_view("abc", 3), ec);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        fx.server_conn.send(const_buffer_view("def", 3), ec);
    });

    std::array<char, 6> buf;
    std::atomic<int> calls{0};
    std::string got;
    std::atomic<bool> done{false};

    async_read(fx.client, buffer_view(buf),
        [&](lcy::system::error_code ec, std::size_t n) {
            ++calls;
            if ( !ec ) got.assign(buf.data(), n);
            done = true;
            fx.ioc.stop();
        });

    fx.start_runners();
    writer.join();

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while ( !done && std::chrono::steady_clock::now() < deadline ) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    assert(done);
    assert(calls == 1);
    assert(got == "abcdef");
    std::cout << "  async read 6 bytes in 1 callback ok\n";
}

static void test_async_read_eof()
{
    std::cout << "[test_async_read_eof]\n";
    tcp_pair fx(21009);

    std::thread writer([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        lcy::system::error_code ec;
        fx.server_conn.send(const_buffer_view("ab", 2), ec);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        fx.server_conn.close(ec);
    });

    std::array<char, 6> buf;
    std::atomic<bool> done{false};
    std::size_t got_n = 0;
    lcy::system::error_code got_ec;

    async_read(fx.client, buffer_view(buf),
        [&](lcy::system::error_code ec, std::size_t n) {
            got_n = n;
            got_ec = ec;
            done = true;
            fx.ioc.stop();
        });

    fx.start_runners();
    writer.join();

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while ( !done && std::chrono::steady_clock::now() < deadline ) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    assert(done);
    assert(got_ec);
    assert(got_n == 2);
    std::cout << "  async read eof after 2 bytes, ec=" << got_ec.message() << "\n";
}

static void test_async_write_full()
{
    std::cout << "[test_async_write_full]\n";
    tcp_pair fx(21010);

    std::string msg(2000, 'z');
    std::atomic<bool> done{false};
    std::size_t sent_total = 0;
    lcy::system::error_code sent_ec;

    async_write(fx.client, const_buffer_view(msg.data(), msg.size()),
        [&](lcy::system::error_code ec, std::size_t n) {
            sent_ec = ec;
            if ( !ec ) sent_total = n;
            done = true;
            fx.ioc.stop();
        });

    fx.start_runners();

    std::string recv(2000, '\0');
    std::size_t got = 0;
    while ( got < 2000 ) {
        lcy::system::error_code ec2;
        std::size_t r = fx.server_conn.receive(
            mutable_buffer_view(&recv[got], 2000 - got), ec2);
        if ( ec2 ) break;
        got += r;
    }

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while ( !done && std::chrono::steady_clock::now() < deadline ) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    assert(done);
    assert(!sent_ec);
    assert(sent_total == 2000);
    assert(got == 2000);
    assert(recv == msg);
    std::cout << "  async write 2000 bytes ok\n";
}

static void test_async_write_small()
{
    std::cout << "[test_async_write_small]\n";
    tcp_pair fx(21011);

    std::atomic<bool> done{false};
    std::size_t sent_total = 0;

    async_write(fx.client, const_buffer_view("abc", 3),
        [&](lcy::system::error_code ec, std::size_t n) {
            if ( !ec ) sent_total = n;
            done = true;
            fx.ioc.stop();
        });

    fx.start_runners();

    std::array<char, 3> buf;
    std::size_t got = 0;
    while ( got < 3 ) {
        lcy::system::error_code ec2;
        std::size_t r = fx.server_conn.receive(
            mutable_buffer_view(&buf[got], 3 - got), ec2);
        if ( ec2 ) break;
        got += r;
    }

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while ( !done && std::chrono::steady_clock::now() < deadline ) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    assert(done);
    assert(sent_total == 3);
    assert(got == 3);
    assert(std::string(buf.data(), 3) == "abc");
    std::cout << "  async write 3 bytes ok\n";
}

int main()
{
    test_sync_read_full();
    test_sync_read_partial_then_full();
    test_sync_read_eof();
    test_sync_read_exception();

    test_sync_write_full();
    test_sync_write_small();

    test_async_read_full();
    test_async_read_partial_then_full();
    test_async_read_eof();

    test_async_write_full();
    test_async_write_small();

    std::cout << "\nall stream io tests passed\n";
    return 0;
}