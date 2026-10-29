// test_thread_pool.cc
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstring>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "lcy/asio/io_context.hpp"
#include "lcy/asio/work_guard.hpp"

using namespace lcy::asio;

class thread_pool
{
public:
    explicit thread_pool(std::size_t thread_num)
        : index_(0)
    {
        for ( std::size_t i = 0; i < thread_num; ++i ) {
            workers.emplace_back(new io_context(2));
            work_guards.emplace_back(new work_guard(*workers[i]));
        }
    }

    ~thread_pool()
    {
        stop();
    }

    void start()
    {
        for ( std::size_t i = 0; i < workers.size(); ++i ) {
            threads_.emplace_back(new std::thread([i, this]() {
                workers[i]->run();
            }));
        }
    }

    void stop()
    {
        if ( stopped_ ) return;
        stopped_ = true;

        for ( auto& ptr : work_guards ) {
            ptr->reset();
        }
        for ( auto& th : threads_ ) {
            if ( th->joinable() ) th->join();
        }
    }

    void post(std::function<void()> func)
    {
        std::size_t i = index_.fetch_add(1, std::memory_order_relaxed)
            % workers.size();
        lcy::asio::post(*workers[i], std::move(func));
    }

private:
    std::atomic<bool> stopped_{false};
    std::atomic<std::size_t> index_;
    std::vector<std::unique_ptr<std::thread> > threads_;
    std::vector<std::unique_ptr<io_context> > workers;
    std::vector<std::unique_ptr<work_guard> > work_guards;
};

static void test_basic()
{
    std::cout << "[test_basic]\n";
    thread_pool pool(4);
    pool.start();

    std::atomic<int> count{0};
    for ( int i = 0; i < 100; ++i ) {
        pool.post([&count]() { ++count; });
    }

    auto deadline = std::chrono::steady_clock::now()
        + std::chrono::seconds(5);
    while ( count < 100
            && std::chrono::steady_clock::now() < deadline ) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    assert(count == 100);
    pool.stop();
    std::cout << "  100 tasks ok\n";
}

static void test_large()
{
    std::cout << "[test_large]\n";
    thread_pool pool(5);
    pool.start();

    std::atomic<int> count{0};
    const int N = 10000;
    for ( int i = 0; i < N; ++i ) {
        pool.post([&count]() { ++count; });
    }

    auto deadline = std::chrono::steady_clock::now()
        + std::chrono::seconds(10);
    while ( count < N
            && std::chrono::steady_clock::now() < deadline ) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    assert(count == N);
    pool.stop();
    std::cout << "  " << N << " tasks ok\n";
}

static void test_work_guard_keeps_alive()
{
    std::cout << "[test_work_guard_keeps_alive]\n";
    io_context ioc(2);
    {
        work_guard guard(ioc);

        std::atomic<bool> done{false};
        std::thread t([&]() {
            ioc.run();
            done = true;
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        assert(!done);

        std::atomic<bool> executed{false};
        post(ioc, [&]() { executed = true; });

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        assert(executed);
        assert(!done);

        guard.reset();
        t.join();
        assert(done);
    }
    std::cout << "  work_guard ok\n";
}

static void test_concurrent_post()
{
    std::cout << "[test_concurrent_post]\n";
    thread_pool pool(4);
    pool.start();

    const int THREADS = 4;
    const int PER_THREAD = 1000;
    const int TOTAL = THREADS * PER_THREAD;
    std::atomic<int> count{0};

    std::vector<std::thread> posters;
    for ( int i = 0; i < THREADS; ++i ) {
        posters.emplace_back([&]() {
            for ( int j = 0; j < PER_THREAD; ++j ) {
                pool.post([&count]() { ++count; });
            }
        });
    }
    for ( auto& t : posters ) t.join();

    auto deadline = std::chrono::steady_clock::now()
        + std::chrono::seconds(10);
    while ( count < TOTAL
            && std::chrono::steady_clock::now() < deadline ) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    assert(count == TOTAL);
    pool.stop();
    std::cout << "  " << TOTAL << " tasks from " << THREADS
              << " threads ok\n";
}

static void test_stop_idempotent()
{
    std::cout << "[test_stop_idempotent]\n";
    thread_pool pool(2);
    pool.start();

    pool.post([]() {});
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    pool.stop();
    pool.stop();   // 第二次幂等
    pool.stop();
    std::cout << "  stop idempotent ok\n";
}

static void test_empty_pool()
{
    std::cout << "[test_empty_pool]\n";
    {
        thread_pool pool(3);
    }
    std::cout << "  empty pool ok\n";
}

static void test_post_then_stop()
{
    std::cout << "[test_post_then_stop]\n";
    thread_pool pool(2);
    pool.start();

    std::atomic<int> count{0};
    for ( int i = 0; i < 100; ++i ) {
        pool.post([&count]() { ++count; });
    }
    pool.stop();

    std::cout << "  count after stop = " << count << "\n";
    assert(count == 100);
    std::cout << "  post-then-stop ok\n";
}

int main()
{
    test_basic();
    test_large();
    test_work_guard_keeps_alive();
    test_concurrent_post();
    test_stop_idempotent();
    test_empty_pool();
    test_post_then_stop();

    std::cout << "\nall thread_pool tests passed\n";
    return 0;
}