// test_scheduler.cc
#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "lcy/asio/detail/scheduler.hpp"

using namespace lcy::asio::detail;

template <typename Fn>
static void run_with_timeout(scheduler& sch, Fn fn, int timeout_ms,
                             const char* name)
{
    std::atomic<bool> done{false};
    std::thread t([&]() {
        fn();
        done = true;
    });

    auto deadline = std::chrono::steady_clock::now()
        + std::chrono::milliseconds(timeout_ms);
    while ( !done && std::chrono::steady_clock::now() < deadline ) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    if ( !done ) {
        std::cerr << "TIMEOUT in " << name << "\n";
        sch.stop();
        t.detach();
        std::abort();
    }
    t.join();
}

static void test_run_empty()
{
    std::cout << "[test_run_empty]\n";
    scheduler sch(1);

    // pending_operations_ == 0，run() 开头直接 stop + return 0
    std::size_t n = sch.run();
    assert(n == 0);
    std::cout << "  run() on empty returned 0\n";
}

static void test_single_op()
{
    std::cout << "[test_single_op]\n";
    scheduler sch(1);

    bool executed = false;
    sch.operation_increase();
    sch.submit(operation([&executed](lcy::system::error_code ec, int) {
        (void)ec;
        executed = true;
    }));

    std::size_t n = sch.run();
    assert(n == 1);
    assert(executed);
    std::cout << "  single op executed, count=1\n";
}

static void test_multi_ops()
{
    std::cout << "[test_multi_ops]\n";
    scheduler sch(1);

    const int N = 100;
    std::atomic<int> executed{0};

    for ( int i = 0; i < N; ++i ) {
        sch.post(operation([&executed](lcy::system::error_code, int) {
            ++executed;
        }));
    }

    std::size_t n = sch.run();
    assert(n == N);
    assert(executed == N);
    std::cout << "  " << N << " ops executed\n";
}

static void test_stop()
{
    std::cout << "[test_stop]\n";
    scheduler sch(1);

    std::atomic<int> executed{0};
    for ( int i = 0; i < 10; ++i ) {
        sch.post(operation([&executed](lcy::system::error_code, int) {
            ++executed;
        }));
    }

    std::thread runner([&]() { sch.run(); });

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    sch.stop();
    runner.join();

    std::cout << "  stop after " << executed << "/10 ops\n";
    assert(executed <= 10);
}

static void test_stop_idempotent()
{
    std::cout << "[test_stop_idempotent]\n";
    scheduler sch(1);

    sch.stop();
    sch.stop();
    sch.stop();

    std::size_t n = sch.run();   // stopped_ 已 true
    assert(n == 0);
    std::cout << "  repeated stop ok\n";
}

static void test_operation_count()
{
    std::cout << "[test_operation_count]\n";
    scheduler sch(1);

    sch.operation_increase();
    sch.operation_increase();
    sch.operation_increase();

    sch.operation_decrease();
    sch.operation_decrease();
    sch.operation_decrease();

    std::size_t n = sch.run();
    assert(n == 0);
    std::cout << "  increase/decrease paired, stop on zero\n";
}

static void test_post_from_op()
{
    std::cout << "[test_post_from_op]\n";
    scheduler sch(1);

    std::atomic<int> executed{0};
    const int N = 5;

    for ( int i = 0; i < N; ++i ) {
        sch.post(operation([&](lcy::system::error_code, int) {
            ++executed;
            if ( executed < N ) {
                sch.post(operation([&](lcy::system::error_code, int) {
                    ++executed;
                }));
            }
        }));
    }

    std::size_t n = sch.run();
    assert(executed >= N);
    std::cout << "  post-from-op executed=" << executed
              << " count=" << n << "\n";
}

static void test_multithread()
{
    std::cout << "[test_multithread]\n";
    const std::size_t THREADS = 4;
    const int OPS_PER_THREAD = 1000;
    const int TOTAL = THREADS * OPS_PER_THREAD;

    scheduler sch(THREADS);
    std::atomic<int> executed{0};

    sch.operation_increase();

    std::vector<std::thread> runners;
    for ( std::size_t i = 0; i < THREADS; ++i ) {
        runners.emplace_back([&]() { sch.run(); });
    }

    std::vector<std::thread> posters;
    for ( std::size_t i = 0; i < THREADS; ++i ) {
        posters.emplace_back([&]() {
            for ( int j = 0; j < OPS_PER_THREAD; ++j ) {
                sch.post(operation([&executed](lcy::system::error_code, int) {
                    ++executed;
                }));
            }
        });
    }
    for ( auto& t : posters ) t.join();

    auto deadline = std::chrono::steady_clock::now()
        + std::chrono::seconds(10);
    while ( executed < TOTAL
            && std::chrono::steady_clock::now() < deadline ) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    sch.stop();
    sch.operation_decrease();
    for ( auto& t : runners ) t.join();

    assert(executed == TOTAL);
    std::cout << "  " << TOTAL << " ops across " << THREADS
              << " threads executed=" << executed << "\n";
}

static void test_reactor_init()
{
    std::cout << "[test_reactor_init]\n";
    scheduler sch(1);

    reactor& r = sch.get_reactor();
    (void)r;

    std::size_t n = sch.run();
    assert(n == 0);
    std::cout << "  reactor init ok, run returned 0\n";
}

static void test_cleanup()
{
    std::cout << "[test_cleanup]\n";
    scheduler sch(1);

    sch.post(operation([](lcy::system::error_code, int) {}));
    sch.post(operation([](lcy::system::error_code, int) {}));

    sch.cleanup();
    std::cout << "  cleanup ok\n";
}

int main()
{
    test_run_empty();
    test_single_op();
    test_multi_ops();
    test_stop();
    test_stop_idempotent();
    test_operation_count();
    test_post_from_op();
    test_multithread();
    test_reactor_init();
    test_cleanup();

    std::cout << "\nall scheduler tests passed\n";
    return 0;
}