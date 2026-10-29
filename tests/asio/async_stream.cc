// test_async_stream.cc
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
#include "lcy/asio/async_stream.hpp"

using namespace lcy::asio;

struct server_fixture
{
    io_context ioc;
    work_guard guard;
    ip::tcp::acceptor acceptor;
    ip::tcp::socket conn;
    ip::tcp::endpoint ep;
    std::vector<std::thread> runners;

    server_fixture(std::size_t nthreads, unsigned short port)
        : ioc(nthreads), guard(ioc), acceptor(ioc), conn(ioc),
          ep(ip::tcp::v4(), port)
    {
        acceptor.open(ip::tcp::v4());
        acceptor.bind(ep);
        acceptor.listen(1);
        for ( std::size_t i = 0; i < nthreads; ++i ) {
            runners.emplace_back([this]() { ioc.run(); });
        }
    }

    ~server_fixture()
    {
        ioc.stop();
        for ( auto& t : runners ) if ( t.joinable() ) t.join();
    }
};

static void test_async_read_partial()
{
    std::cout << "[test_async_read_partial]\n";
    server_fixture fx(2, 20001);

    ip::tcp::socket client(fx.ioc);
    client.open(ip::tcp::v4());
    lcy::system::error_code ec;
    client.connect(fx.ep, ec);
    assert(!ec);

    ip::tcp::socket server_conn(fx.ioc);
    ip::tcp::endpoint peer;
    fx.acceptor.accept(server_conn, peer, ec);
    assert(!ec);

    std::thread writer([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        server_conn.send(const_buffer_view("abc", 3));
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        server_conn.send(const_buffer_view("def", 3));
    });

    std::array<char, 6> buf;
    std::atomic<int> calls{0};
    std::string got;
    async_read(client, buffer_view(buf),
        [&](lcy::system::error_code ec, std::size_t n) {
            ++calls;
            if ( !ec ) got.assign(buf.data(), n);
            fx.ioc.stop();
        });

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while ( calls == 0 && std::chrono::steady_clock::now() < deadline ) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    writer.join();

    assert(calls == 1);
    assert(got == "abcdef");
    std::cout << "  got 'abcdef' in 1 callback\n";
}

static void test_async_read_eof()
{
    std::cout << "[test_async_read_eof]\n";
    server_fixture fx(2, 20002);

    ip::tcp::socket client(fx.ioc);
    client.open(ip::tcp::v4());
    lcy::system::error_code ec;
    client.connect(fx.ep, ec);
    assert(!ec);

    ip::tcp::socket server_conn(fx.ioc);
    ip::tcp::endpoint peer;
    fx.acceptor.accept(server_conn, peer, ec);
    assert(!ec);

    std::thread writer([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        server_conn.send(const_buffer_view("ab", 2));
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        server_conn.close();
    });

    std::array<char, 6> buf;
    std::atomic<bool> done{false};
    std::size_t got_n = 0;
    lcy::system::error_code got_ec;
    async_read(client, buffer_view(buf),
        [&](lcy::system::error_code ec, std::size_t n) {
            got_n = n;
            got_ec = ec;
            done = true;
            fx.ioc.stop();
        });

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while ( !done && std::chrono::steady_clock::now() < deadline ) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    writer.join();

    assert(done);
    assert(got_ec);
    assert(got_n == 2);
    std::cout << "  eof after 2 bytes, ec=" << got_ec.message() << "\n";
}

static void test_async_write_full()
{
    std::cout << "[test_async_write_full]\n";
    server_fixture fx(2, 20003);

    ip::tcp::socket client(fx.ioc);
    client.open(ip::tcp::v4());
    lcy::system::error_code ec;
    client.connect(fx.ep, ec);
    assert(!ec);

    ip::tcp::socket server_conn(fx.ioc);
    ip::tcp::endpoint peer;
    fx.acceptor.accept(server_conn, peer, ec);
    assert(!ec);

    std::string msg(2000, 'z');
    std::atomic<bool> done{false};
    std::size_t sent_total = 0;

    async_write(client, const_buffer_view(msg.data(), msg.size()),
        [&](lcy::system::error_code ec, std::size_t n) {
            if ( !ec ) sent_total = n;
            done = true;
            fx.ioc.stop();
        });

    std::string recv(2000, '\0');
    std::size_t got = 0;
    while ( got < 2000 ) {
        lcy::system::error_code ec2;
        std::size_t n = server_conn.receive(
            mutable_buffer_view(&recv[got], 2000 - got), ec2);
        if ( ec2 ) break;
        got += n;
    }

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while ( !done && std::chrono::steady_clock::now() < deadline ) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    assert(done);
    assert(sent_total == 2000);
    assert(got == 2000);
    assert(recv == msg);
    std::cout << "  2000 bytes written and received\n";
}

int main()
{
    test_async_read_partial();
    test_async_read_eof();
    test_async_write_full();
    std::cout << "\nall async_stream tests passed\n";
    return 0;
}