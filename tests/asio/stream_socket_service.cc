// test_stream_socket_service.cc
#include <cassert>
#include <iostream>
#include <thread>
#include <atomic>

#include "lcy/asio/ip/tcp.hpp"
#include "lcy/asio/buffer_view.hpp"
#include "lcy/asio/ip/socket_option.hpp"
#include "lcy/asio/detail/scheduler.hpp"
#include "lcy/asio/detail/stream_socket_service.hpp"

using namespace lcy::asio;
using namespace lcy::asio::detail;

static const unsigned short BASE_PORT = 34567;

static void test_sync_flow()
{
    std::cout << "[test_sync_flow]\n";
    lcy::system::error_code ec;
    scheduler sch(1);
    stream_socket_service service(sch.get_reactor());
    stream_socket_service::socket_impl server, client;

    service.construct(server);
    service.construct(client);

    service.open(server, ip::tcp::v4(), ec);
    assert(!ec);

    ip::tcp::endpoint ep("127.0.0.1", BASE_PORT);
    service.bind(server, ep, ec);
    assert(!ec);

    service.listen(server, 1, ec);
    assert(!ec);

    service.open(client, ip::tcp::v4(), ec);
    assert(!ec);

    service.connect(client, ep, ec);
    assert(!ec);

    service.close(client, ec);
    service.close(server, ec);
    std::cout << "  sync flow ok\n";
}

static void test_sync_accept()
{
    std::cout << "[test_sync_accept]\n";
    lcy::system::error_code ec;
    scheduler sch(1);
    stream_socket_service service(sch.get_reactor());
    stream_socket_service::socket_impl server, client, accepted;

    service.construct(server);
    service.construct(client);
    service.construct(accepted);

    service.open(server, ip::tcp::v4(), ec);
    ip::tcp::endpoint ep("127.0.0.1", BASE_PORT + 1);
    service.bind(server, ep, ec);
    service.listen(server, 1, ec);

    service.open(client, ip::tcp::v4(), ec);
    service.connect(client, ep, ec);

    ip::tcp::endpoint peer;
    service.accept(server, accepted, peer, ec);
    assert(!ec);
    assert(service.is_open(accepted));

    service.close(accepted, ec);
    service.close(client, ec);
    service.close(server, ec);
    std::cout << "  sync accept ok\n";
}

static void test_sync_send_receive()
{
    std::cout << "[test_sync_send_receive]\n";
    lcy::system::error_code ec;
    scheduler sch(1);
    stream_socket_service service(sch.get_reactor());
    stream_socket_service::socket_impl server, client, accepted;

    service.construct(server);
    service.construct(client);
    service.construct(accepted);

    service.open(server, ip::tcp::v4(), ec);
    ip::tcp::endpoint ep("127.0.0.1", BASE_PORT + 2);
    service.bind(server, ep, ec);
    service.listen(server, 1, ec);

    service.open(client, ip::tcp::v4(), ec);
    service.connect(client, ep, ec);

    ip::tcp::endpoint peer;
    service.accept(server, accepted, peer, ec);

    const char msg[] = "hello!";
    std::size_t sent = service.send(client,
        const_buffer_view(msg, sizeof(msg) - 1), ec);
    assert(!ec);
    assert(sent == sizeof(msg) - 1);

    char buf[16] = {0};
    std::size_t recvd = service.receive(accepted,
        mutable_buffer_view(buf, sizeof(buf)), ec);
    assert(!ec);
    assert(recvd == sizeof(msg) - 1);
    assert(std::string(buf, recvd) == "hello!");

    service.close(accepted, ec);
    service.close(client, ec);
    service.close(server, ec);
    std::cout << "  send/receive ok\n";
}

static void test_sync_options()
{
    std::cout << "[test_sync_options]\n";
    lcy::system::error_code ec;
    scheduler sch(1);
    stream_socket_service service(sch.get_reactor());
    stream_socket_service::socket_impl s;
    service.construct(s);
    service.open(s, ip::tcp::v4(), ec);
    assert(!ec);

    lcy::asio::ip::socket_option::tcp_nodelay opt;
    service.get_option(s, opt, ec);
    assert(!ec);

    lcy::asio::ip::socket_option::tcp_nodelay opt_set(1);
    service.set_option(s, opt_set, ec);
    assert(!ec);

    lcy::asio::ip::socket_option::tcp_nodelay opt2;
    service.get_option(s, opt2, ec);
    assert(!ec);
    assert(opt2.value() == 1);

    service.close(s, ec);
    std::cout << "  options ok\n";
}

static void test_async()
{
    std::cout << "[test_async]\n";
    lcy::system::error_code ec;
    scheduler sch(1);
    stream_socket_service service(sch.get_reactor());
    stream_socket_service::socket_impl server, client, accepted;

    service.construct(server);
    service.construct(client);
    service.construct(accepted);

    service.open(server, ip::tcp::v4(), ec);
    ip::tcp::endpoint ep("127.0.0.1", BASE_PORT + 3);
    service.bind(server, ep, ec);
    service.listen(server, 1, ec);

    service.open(client, ip::tcp::v4(), ec);

    bool connect_done = false;
    service.async_connect(client, ep, [&](lcy::system::error_code ec) {
        if ( !ec ) connect_done = true;
    });

    sch.run();

    assert(connect_done);

    service.close(client, ec);
    service.close(server, ec);
    std::cout << "  async_connect ok\n";
}

static void test_async_accept()
{
    std::cout << "[test_async_accept]\n";
    lcy::system::error_code ec;
    scheduler sch(1);
    stream_socket_service service(sch.get_reactor());
    stream_socket_service::socket_impl server, client, accepted;

    service.construct(server);
    service.construct(client);
    service.construct(accepted);

    service.open(server, ip::tcp::v4(), ec);
    ip::tcp::endpoint ep("127.0.0.1", BASE_PORT + 4);
    service.bind(server, ep, ec);
    service.listen(server, 1, ec);

    service.open(client, ip::tcp::v4(), ec);
    service.connect(client, ep, ec);
    assert(!ec);

    ip::tcp::endpoint peer;
    bool accept_done = false;
    service.async_accept(server, accepted, peer,
        [&](lcy::system::error_code ec) {
            if ( !ec ) accept_done = true;
        });

    sch.run();

    assert(accept_done);
    assert(service.is_open(accepted));

    service.close(accepted, ec);
    service.close(client, ec);
    service.close(server, ec);
    std::cout << "  async_accept ok\n";
}

static void test_connect_refused()
{
    std::cout << "[test_connect_refused]\n";
    lcy::system::error_code ec;
    scheduler sch(1);
    stream_socket_service service(sch.get_reactor());
    stream_socket_service::socket_impl client;

    service.construct(client);
    service.open(client, ip::tcp::v4(), ec);
    assert(!ec);

    ip::tcp::endpoint ep("127.0.0.1", 1);
    service.connect(client, ep, ec);
    assert(ec);
    std::cout << "  expected error: " << ec.message() << "\n";

    service.close(client, ec);
}

static void test_not_open()
{
    std::cout << "[test_not_open]\n";
    lcy::system::error_code ec;
    scheduler sch(1);
    stream_socket_service service(sch.get_reactor());
    stream_socket_service::socket_impl s;
    service.construct(s);

    char buf[4];
    std::size_t n = service.receive(s, mutable_buffer_view(buf, sizeof(buf)), ec);
    assert(ec);
    assert(n == 0);

    std::cout << "  expected error: " << ec.message() << "\n";
}

static void test_already_open()
{
    std::cout << "[test_already_open]\n";
    lcy::system::error_code ec;
    scheduler sch(1);
    stream_socket_service service(sch.get_reactor());
    stream_socket_service::socket_impl s;
    service.construct(s);

    service.open(s, ip::tcp::v4(), ec);
    assert(!ec);

    service.open(s, ip::tcp::v4(), ec);
    assert(ec);

    service.close(s, ec);
    std::cout << "  expected error: " << ec.message() << "\n";
}

int main()
{
    test_sync_flow();
    test_sync_accept();
    test_sync_send_receive();
    test_sync_options();
    test_async();
    test_async_accept();
    test_connect_refused();
    test_not_open();
    test_already_open();

    std::cout << "\nall stream_socket_service tests passed\n";
    return 0;
}