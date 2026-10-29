// test_steady_timer.cc
#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "lcy/asio/io_context.hpp"
#include "lcy/asio/steady_timer.hpp"

using namespace lcy::asio;

static long long now_ms()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

static void test_basic()
{
    std::cout << "[test_basic]\n";
    io_context ioc(1);
    steady_timer timer(ioc);

    bool fired = false;
    timer.expires_after(50);
    timer.async_wait([&](lcy::system::error_code ec) {
        if ( !ec ) fired = true;
        ioc.stop();
    });

    long long t0 = now_ms();
    ioc.run();
    long long elapsed = now_ms() - t0;

    assert(fired);
    assert(elapsed >= 50);
    std::cout << "  fired after " << elapsed << " ms\n";
}

static void test_expires_at()
{
    std::cout << "[test_expires_at]\n";
    io_context ioc(1);
    steady_timer timer(ioc);

    bool fired = false;
    long long target = steady_timer::clock::now() + 50;
    timer.expires_at(target);
    timer.async_wait([&](lcy::system::error_code ec) {
        if ( !ec ) fired = true;
        ioc.stop();
    });

    long long t0 = now_ms();
    ioc.run();
    long long elapsed = now_ms() - t0;

    assert(fired);
    assert(elapsed >= 50);
    std::cout << "  fired after " << elapsed << " ms\n";
}

static void test_cancel()
{
    std::cout << "[test_cancel]\n";
    io_context ioc(1);
    steady_timer timer(ioc);

    bool fired = false;
    bool cancelled = false;
    timer.expires_after(10000);
    timer.async_wait([&](lcy::system::error_code ec) {
        if ( ec ) cancelled = true;
        else      fired = true;
        ioc.stop();
    });

    timer.cancel();
    ioc.run();

    assert(cancelled);
    assert(!fired);
    std::cout << "  cancelled ok\n";
}

static void test_expiry()
{
    std::cout << "[test_expiry]\n";
    io_context ioc(1);
    steady_timer timer(ioc);

    timer.expires_after(100);
    long long r1 = timer.expiry();
    assert(r1 > 0 && r1 <= 100);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    long long r2 = timer.expiry();
    assert(r2 < r1);

    std::cout << "  expiry: " << r1 << " -> " << r2 << " ms\n";
}

static void test_sync_wait()
{
    std::cout << "[test_sync_wait]\n";
    io_context ioc(1);
    steady_timer timer(ioc);

    timer.expires_after(30);
    lcy::system::error_code ec;
    long long t0 = now_ms();
    timer.wait(ec);
    long long elapsed = now_ms() - t0;

    assert(!ec);
    assert(elapsed >= 30);
    std::cout << "  sync wait " << elapsed << " ms\n";
}

static void test_multiple_timers()
{
    std::cout << "[test_multiple_timers]\n";
    io_context ioc(1);
    steady_timer t1(ioc), t2(ioc), t3(ioc);

    std::vector<int> order;
    std::atomic<int> done{0};

    auto cb = [&](int id) {
        return [&, id](lcy::system::error_code ec) {
            if ( !ec ) order.push_back(id);
            if ( ++done == 3 ) ioc.stop();
        };
    };

    t1.expires_after(20);  t1.async_wait(cb(1));
    t2.expires_after(40);  t2.async_wait(cb(2));
    t3.expires_after(60);  t3.async_wait(cb(3));

    ioc.run();

    assert(done == 3);
    assert(order.size() == 3);
    assert(order[0] == 1);
    assert(order[1] == 2);
    assert(order[2] == 3);
    std::cout << "  order: ";
    for ( int id : order ) std::cout << id << " ";
    std::cout << "\n";
}

static void test_destroy_without_fire()
{
    std::cout << "[test_destroy_without_fire]\n";
    io_context ioc(1);
    {
        steady_timer timer(ioc);
        timer.expires_after(10000);
        timer.async_wait([](lcy::system::error_code) {});
    }
    std::atomic<bool> done{false};
    post(ioc, [&]() { done = true; ioc.stop(); });
    ioc.run();
    assert(done);
    std::cout << "  destroy without fire ok\n";
}

static void test_run_with_timeout()
{
    std::cout << "[test_run_with_timeout]\n";
    io_context ioc(1);
    steady_timer timer(ioc);

    bool fired = false;
    timer.expires_after(50);
    timer.async_wait([&](lcy::system::error_code ec) {
        if ( !ec ) fired = true;
    });

    std::atomic<bool> done{false};
    std::thread watchdog([&]() {
        auto deadline = std::chrono::steady_clock::now()
            + std::chrono::seconds(5);
        while ( !done && std::chrono::steady_clock::now() < deadline ) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        if ( !done ) {
            std::cerr << "  TIMEOUT, stopping\n";
            ioc.stop();
        }
    });

    ioc.run();
    done = true;
    watchdog.join();

    assert(fired);
    std::cout << "  run with timeout ok\n";
}

static void test_overwrite_wait()
{
    std::cout << "[test_overwrite_wait]\n";
    io_context ioc(1);
    steady_timer timer(ioc);

    int calls = 0;
    timer.expires_after(50);
    timer.async_wait([&](lcy::system::error_code ec) {
        if ( !ec ) ++calls;
    });
    timer.expires_after(20);
    timer.async_wait([&](lcy::system::error_code ec) {
        if ( !ec ) ++calls;
        ioc.stop();
    });

    ioc.run();
    std::cout << "  calls=" << calls << "\n";
    assert(calls >= 1);
}

int main()
{
    test_basic();
    test_expires_at();
    test_cancel();
    test_expiry();
    test_sync_wait();
    test_multiple_timers();
    test_destroy_without_fire();
    test_run_with_timeout();
    test_overwrite_wait();

    std::cout << "\nall steady_timer tests passed\n";
    return 0;
}