// test_strand.cc
#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "lcy/asio/io_context.hpp"
#include "lcy/asio/strand.hpp"

using namespace lcy::asio;

static void test_no_strand()
{
    std::cout << "[test_no_strand]\n";
    io_context ioc;
    int count = 0;
    const int N = 100000;

    for ( int i = 0; i < N; ++i ) {
        post(ioc, [&count]() {
            int tmp = count;
            tmp = count + 1;
            count = tmp;
        });
    }

    std::vector<std::thread> threads;
    for ( int i = 0; i < 5; ++i ) {
        threads.emplace_back([&ioc]() { ioc.run(); });
    }
    for ( auto& t : threads ) t.join();

    std::cout << "  count = " << count << " (expected ~" << N << ", race)\n";
    assert(count >= 0 && count <= N);
}

static void test_strand()
{
    std::cout << "[test_strand]\n";
    io_context ioc;
    strand s(ioc);
    int count = 0;
    const int N = 100000;

    for ( int i = 0; i < N; ++i ) {
        s.post([&count]() {
            int tmp = count;
            tmp = count + 1;
            count = tmp;
        });
    }

    std::vector<std::thread> threads;
    for ( int i = 0; i < 5; ++i ) {
        threads.emplace_back([&ioc]() { ioc.run(); });
    }
    for ( auto& t : threads ) t.join();

    assert(count == N);
    std::cout << "  count = " << count << "\n";
}

static void test_strand_wrapper()
{
    std::cout << "[test_strand_wrapper]\n";
    io_context ioc;
    strand s(ioc);

    int result = 0;
    auto func = strand_wrapper(s, [](int a, int b, int c) {
        static int unused = 0;
        (void)unused;
        std::cout << "  strand_wrapper: " << a + b + c << "\n";
    });

    func(10, 20, 30);
    ioc.run();
    (void)result;
}

static void test_serial()
{
    std::cout << "[test_serial]\n";
    io_context ioc;
    strand s(ioc);
    std::vector<int> log;
    std::mutex log_mutex;

    for ( int i = 0; i < 1000; ++i ) {
        s.post([&, i]() {
            std::lock_guard<std::mutex> lk(log_mutex);
            log.push_back(i);
        });
    }

    std::vector<std::thread> threads;
    for ( int i = 0; i < 4; ++i ) {
        threads.emplace_back([&ioc]() { ioc.run(); });
    }
    for ( auto& t : threads ) t.join();

    assert(log.size() == 1000);
    for ( int i = 0; i < 1000; ++i ) {
        assert(log[i] == i);
    }
    std::cout << "  serial order ok, 1000 posts\n";
}

static void test_nested_post()
{
    std::cout << "[test_nested_post]\n";
    io_context ioc;
    strand s(ioc);

    std::atomic<int> count{0};
    s.post([&]() {
        ++count;
        s.post([&]() {
            ++count;
            s.post([&]() {
                ++count;
            });
        });
    });

    ioc.run();
    std::cout << "  nested count = " << count << "\n";
    assert(count >= 1);
}

static void test_multi_strand()
{
    std::cout << "[test_multi_strand]\n";
    io_context ioc;
    strand s1(ioc), s2(ioc);
    int c1 = 0, c2 = 0;
    const int N = 10000;

    for ( int i = 0; i < N; ++i ) {
        s1.post([&c1]() { ++c1; });
        s2.post([&c2]() { ++c2; });
    }

    std::vector<std::thread> threads;
    for ( int i = 0; i < 4; ++i ) {
        threads.emplace_back([&ioc]() { ioc.run(); });
    }
    for ( auto& t : threads ) t.join();

    assert(c1 == N);
    assert(c2 == N);
    std::cout << "  c1=" << c1 << " c2=" << c2 << "\n";
}

int main()
{
    test_no_strand();
    test_strand();
    test_strand_wrapper();
    test_serial();
    test_nested_post();
    test_multi_strand();

    std::cout << "\nall strand tests passed\n";
    return 0;
}