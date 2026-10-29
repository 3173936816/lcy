// test_steady_timer_all.cc
#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "lcy/asio/io_context.hpp"
#include "lcy/asio/steady_timer.hpp"
#include "lcy/asio/work_guard.hpp"

using namespace lcy::asio;

static long long now_ms()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

static void test_expires_after()
{
    io_context ioc(1);
    work_guard guard(ioc);

    steady_timer timer(ioc);
    std::atomic<bool> fired{false};
    std::atomic<bool> has_ec{false};

    timer.expires_after(20);
    timer.async_wait([&](lcy::system::error_code ec) {
        fired = true;
        if ( ec ) has_ec = true;
        ioc.stop();
    });

    long long t0 = now_ms();
    ioc.run();
    long long elapsed = now_ms() - t0;

    assert(fired);
    assert(!has_ec);
    assert(elapsed >= 20);
    std::cout << "  expires_after: fired after " << elapsed << " ms\n";
}

static void test_expires_at()
{
    io_context ioc(1);
    work_guard guard(ioc);

    steady_timer timer(ioc);
    std::atomic<bool> fired{false};

    long long target = steady_timer::clock::now() + 30;
    timer.expires_at(target);
    timer.async_wait([&](lcy::system::error_code ec) {
        if ( !ec ) fired = true;
        ioc.stop();
    });

    long long t0 = now_ms();
    ioc.run();
    long long elapsed = now_ms() - t0;

    assert(fired);
    assert(elapsed >= 30);
    std::cout << "  expires_at: fired after " << elapsed << " ms\n";
}

static void test_cancel()
{
    io_context ioc(1);
    work_guard guard(ioc);

    steady_timer timer(ioc);
    std::atomic<bool> fired{false};
    std::atomic<bool> cancelled{false};

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
    std::cout << "  cancel: callback got operation_abort\n";
}

static void test_reset()
{
    io_context ioc(1);
    work_guard guard(ioc);

    steady_timer timer(ioc);
    std::atomic<int> first_calls{0};
    std::atomic<int> second_calls{0};
    std::atomic<bool> first_cancelled{false};
    std::atomic<bool> second_fired{false};

    timer.expires_after(10000);
    timer.async_wait([&](lcy::system::error_code ec) {
        ++first_calls;
        if ( ec ) first_cancelled = true;
    });

    timer.expires_after(10);
    timer.async_wait([&](lcy::system::error_code ec) {
        ++second_calls;
        if ( !ec ) second_fired = true;
        ioc.stop();
    });

    ioc.run();

    assert(second_fired);
    assert(second_calls == 1);
    std::cout << "  reset: second wait fired, first_calls=" << first_calls
              << " first_cancelled=" << first_cancelled << "\n";
}

static void test_expiry()
{
    io_context ioc(1);
    steady_timer timer(ioc);

    timer.expires_after(100);
    long long remain = timer.expiry();
    assert(remain > 0 && remain <= 100);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    long long remain2 = timer.expiry();
    assert(remain2 < remain);

    std::cout << "  expiry: " << remain << " -> " << remain2 << " ms\n";
}

static void test_sync_wait()
{
    io_context ioc(1);
    steady_timer timer(ioc);

    timer.expires_after(10);
    lcy::system::error_code ec;
    long long t0 = now_ms();
    timer.wait(ec);
    long long elapsed = now_ms() - t0;

    assert(!ec);
    assert(elapsed >= 10);
    std::cout << "  sync wait: " << elapsed << " ms\n";
}

static void test_destroy_without_fire()
{
    io_context ioc(1);
    work_guard guard(ioc);

    {
        steady_timer timer(ioc);
        timer.expires_after(10000);
        timer.async_wait([](lcy::system::error_code) {});
    }

    std::atomic<bool> done{false};
    post(ioc, [&done, &ioc]() { done = true; ioc.stop(); });
    ioc.run();

    assert(done);
    std::cout << "  destroy without fire: ok\n";
}

static void test_multiple_timers()
{
    io_context ioc(1);
    work_guard guard(ioc);

    steady_timer t1(ioc), t2(ioc), t3(ioc);
    std::vector<int> order;
    std::atomic<int> done{0};

    auto cb = [&](int id) {
        return [&, id](lcy::system::error_code ec) {
            if ( !ec ) order.push_back(id);
            if ( ++done == 3 ) ioc.stop();
        };
    };

    t1.expires_after(10);  t1.async_wait(cb(1));
    t2.expires_after(30);  t2.async_wait(cb(2));
    t3.expires_after(50);  t3.async_wait(cb(3));

    ioc.run();

    assert(done == 3);
    assert(order.size() == 3);
    assert(order[0] == 1);
    assert(order[1] == 2);
    assert(order[2] == 3);
    std::cout << "  multiple timers order: ";
    for ( int id : order ) std::cout << id << " ";
    std::cout << "\n";
}

int main()
{
    std::cout << "[test_expires_after]\n";        test_expires_after();
    std::cout << "[test_expires_at]\n";           test_expires_at();
    std::cout << "[test_cancel]\n";               test_cancel();
    std::cout << "[test_reset]\n";                test_reset();
    std::cout << "[test_expiry]\n";               test_expiry();
    std::cout << "[test_sync_wait]\n";            test_sync_wait();
    std::cout << "[test_destroy_without_fire]\n"; test_destroy_without_fire();
    std::cout << "[test_multiple_timers]\n";      test_multiple_timers();

    std::cout << "\nall steady_timer tests passed\n";
    return 0;
}