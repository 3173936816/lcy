// test_signal_set.cc
#include <cassert>
#include <chrono>
#include <iostream>
#include <signal.h>
#include <thread>

#include "lcy/asio/io_context.hpp"
#include "lcy/asio/signal_set.hpp"

using namespace lcy::asio;

static void test_basic()
{
    std::cout << "[test_basic]\n";
    io_context ioc(1);
    signal_set sig(ioc, { SIGUSR1 });

    bool fired = false;
    int got = -1;
    sig.async_wait([&](lcy::system::error_code ec, int signum) {
        if ( !ec ) { fired = true; got = signum; }
    });

    ::raise(SIGUSR1);
    ioc.run();

    assert(fired);
    assert(got == SIGUSR1);
    std::cout << "  fired, signum=SIGUSR1\n";
}

static void test_cancel()
{
    std::cout << "[test_cancel]\n";
    io_context ioc(1);
    signal_set sig(ioc, { SIGUSR1 });

    bool cancelled = false;
    bool fired = false;
    sig.async_wait([&](lcy::system::error_code ec, int) {
        if ( ec ) cancelled = true;
        else      fired = true;
    });

    sig.cancel();
    ioc.run();

    assert(cancelled);
    assert(!fired);
    std::cout << "  cancelled ok\n";
}

static void test_multi_signal_set()
{
    std::cout << "[test_multi_signal_set]\n";
    io_context ioc(1);
    signal_set s1(ioc, { SIGUSR1 });
    signal_set s2(ioc, { SIGUSR1 });

    bool f1 = false, f2 = false;
    s1.async_wait([&](lcy::system::error_code ec, int) { if (!ec) f1 = true; });
    s2.async_wait([&](lcy::system::error_code ec, int) { if (!ec) f2 = true; });

    ::raise(SIGUSR1);
    ioc.run();

    assert(f1);
    assert(f2);
    std::cout << "  both fired\n";
}

static void test_multi_signals()
{
    std::cout << "[test_multi_signals]\n";
    io_context ioc(1);
    signal_set s1(ioc, { SIGUSR1 });
    signal_set s2(ioc, { SIGUSR2 });

    bool f1 = false, f2 = false;
    s1.async_wait([&](lcy::system::error_code ec, int) { if (!ec) f1 = true; });
    s2.async_wait([&](lcy::system::error_code ec, int) { if (!ec) f2 = true; });

    ::raise(SIGUSR1);
    ::raise(SIGUSR2);
    ioc.run();

    assert(f1);
    assert(f2);
    std::cout << "  both fired (each own signal)\n";
}

static void test_multi_mask()
{
    std::cout << "[test_multi_mask]\n";
    io_context ioc(1);
    signal_set sig(ioc, { SIGUSR1, SIGUSR2 });

    bool fired = false;
    int got = -1;
    sig.async_wait([&](lcy::system::error_code ec, int signum) {
        if ( !ec ) { fired = true; got = signum; }
    });

    ::raise(SIGUSR2);
    ioc.run();

    assert(fired);
    assert(got == SIGUSR2);
    std::cout << "  fired, got SIGUSR2\n";
}

static void test_destroy_without_fire()
{
    std::cout << "[test_destroy_without_fire]\n";
    io_context ioc(1);
    {
        signal_set sig(ioc, { SIGUSR1 });
        sig.async_wait([](lcy::system::error_code, int) {});
    }
    std::cout << "  destroy without fire ok\n";
}

static void test_multithread_run()
{
    std::cout << "[test_multithread_run]\n";
    io_context ioc(2);
    signal_set sig(ioc, { SIGUSR1 });

    bool fired = false;
    sig.async_wait([&](lcy::system::error_code ec, int) {
        if ( !ec ) fired = true;
    });

    ::raise(SIGUSR1);
    ioc.run();

    assert(fired);
    std::cout << "  multithread run ok\n";
}

static void test_run_with_timeout()
{
    std::cout << "[test_run_with_timeout]\n";
    io_context ioc(1);
    signal_set sig(ioc, { SIGUSR1 });

    bool fired = false;
    sig.async_wait([&](lcy::system::error_code ec, int) {
        if ( !ec ) fired = true;
    });

    // 使用raise是给当前线程发送信号, 信号阻塞在主线程
    // 但是ioc.run跑在子线程里, signalfd读不到信号导致超时
    // 这可能确实是一个不合适的点,因此需要避免这样使用
    // 一般ctrl+c是进程级的(kill触发), 也不太会影响常规使用
    // ::raise(SIGUSR1);

    // 进程级信号
    ::kill(getpid(), SIGUSR1);

    std::atomic<bool> done{false};
    std::thread t([&]() { ioc.run(); done = true; });

    auto deadline = std::chrono::steady_clock::now()
        + std::chrono::seconds(5);
    while ( !done && std::chrono::steady_clock::now() < deadline ) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    if ( !done ) {
        std::cerr << "  TIMEOUT, stopping\n";
        ioc.stop();
        t.join();
        assert(false);
    }
    t.join();

    assert(fired);
    std::cout << "  run with timeout ok\n";
}

int main()
{
    test_basic();
    test_cancel();
    test_multi_signal_set();
    test_multi_signals();
    test_multi_mask();
    test_destroy_without_fire();
    test_multithread_run();
    test_run_with_timeout();

    std::cout << "\nall signal_set tests passed\n";
    return 0;
}