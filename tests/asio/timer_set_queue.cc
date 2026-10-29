// test_timer_set_queue.cc
#include <cassert>
#include <iostream>
#include <list>
#include <unistd.h>
#include <vector>

#include "lcy/asio/detail/basic_timer_set.hpp"
#include "lcy/asio/detail/timer_set_queue.hpp"

using namespace lcy::asio::detail;

struct steady_clock
{
    static long long now()
    {
        timespec ts;
        ::clock_gettime(CLOCK_MONOTONIC, &ts);
        return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    }
};

struct system_clock
{
    static long long now()
    {
        timespec ts;
        ::clock_gettime(CLOCK_REALTIME, &ts);
        return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    }
};

static void test_alloc_free()
{
    std::cout << "[test_alloc_free]\n";
    timer_set_queue tsq;
    auto a = tsq.alloc();
    auto b = tsq.alloc();
    assert(a && b && a != b);
    tsq.free(a);
    tsq.free(b);
    std::cout << "  alloc/free ok\n";
}

static void test_insert_has_remove()
{
    std::cout << "[test_insert_has_remove]\n";
    basic_timer_set<steady_clock> ts;
    auto t = new timer_set_base::internal_timer;

    assert(!ts.has_internal_timer(t));

    bool earliest = ts.insert_internal_timer(t,
        steady_clock::now() + 100,
        operation([](lcy::system::error_code, int) {}));
    assert(earliest);
    assert(ts.has_internal_timer(t));

    bool was_earliest = ts.remove_internal_timer(t);
    assert(was_earliest);
    assert(!ts.has_internal_timer(t));

    delete t;
    std::cout << "  insert/has/remove ok\n";
}

static void test_get_wait_ms()
{
    std::cout << "[test_get_wait_ms]\n";
    basic_timer_set<steady_clock> ts;

    assert(ts.get_wait_ms() == -1);

    auto t = new timer_set_base::internal_timer;
    ts.insert_internal_timer(t, steady_clock::now() + 100,
        operation([](lcy::system::error_code, int) {}));

    long long w = ts.get_wait_ms();
    assert(w > 0 && w <= 100);

    ts.remove_internal_timer(t);
    delete t;
    std::cout << "  get_wait_ms ok\n";
}

static void test_get_ready()
{
    std::cout << "[test_get_ready]\n";
    timer_set_queue tsq;
    basic_timer_set<steady_clock> ts;
    tsq.add_timer_set(&ts);

    auto t = tsq.alloc();
    bool fired = false;
    ts.insert_internal_timer(t, steady_clock::now() + 10,
        operation([&fired](lcy::system::error_code, int) { fired = true; }));

    ::usleep(30 * 1000);

    std::list<operation> ops;
    tsq.get_ready_internal_timer_ops(ops);
    assert(ops.size() == 1);
    for ( auto& op : ops ) op();
    assert(fired);
    assert(!ts.has_internal_timer(t));

    tsq.free(t);
    std::cout << "  get_ready ok\n";
}

static void test_not_expired()
{
    std::cout << "[test_not_expired]\n";
    timer_set_queue tsq;
    basic_timer_set<steady_clock> ts;
    tsq.add_timer_set(&ts);

    auto t = tsq.alloc();
    ts.insert_internal_timer(t, steady_clock::now() + 10000,
        operation([](lcy::system::error_code, int) {}));

    std::list<operation> ops;
    tsq.get_ready_internal_timer_ops(ops);
    assert(ops.empty());
    assert(ts.has_internal_timer(t));

    ts.remove_internal_timer(t);
    tsq.free(t);
    std::cout << "  not expired ok\n";
}

static void test_multi_timers()
{
    std::cout << "[test_multi_timers]\n";
    timer_set_queue tsq;
    basic_timer_set<steady_clock> ts;
    tsq.add_timer_set(&ts);

    auto t1 = tsq.alloc();
    auto t2 = tsq.alloc();
    auto t3 = tsq.alloc();

    std::vector<int> order;
    auto now = steady_clock::now();

    ts.insert_internal_timer(t1, now + 10,
        operation([&order](lcy::system::error_code, int) { order.push_back(1); }));
    ts.insert_internal_timer(t2, now + 20,
        operation([&order](lcy::system::error_code, int) { order.push_back(2); }));
    ts.insert_internal_timer(t3, now + 30,
        operation([&order](lcy::system::error_code, int) { order.push_back(3); }));

    ::usleep(50 * 1000);

    std::list<operation> ops;
    tsq.get_ready_internal_timer_ops(ops);
    assert(ops.size() == 3);
    for ( auto& op : ops ) op();

    assert(order.size() == 3);
    assert(order[0] == 1);
    assert(order[1] == 2);
    assert(order[2] == 3);
    std::cout << "  order: 1 2 3\n";

    tsq.free(t1); tsq.free(t2); tsq.free(t3);
}

static void test_multi_timer_sets()
{
    std::cout << "[test_multi_timer_sets]\n";
    timer_set_queue tsq;
    basic_timer_set<steady_clock> ts1;
    basic_timer_set<system_clock> ts2;
    tsq.add_timer_set(&ts1);
    tsq.add_timer_set(&ts2);

    auto t1 = tsq.alloc();
    auto t2 = tsq.alloc();
    bool f1 = false, f2 = false;

    ts1.insert_internal_timer(t1, steady_clock::now() + 10,
        operation([&f1](lcy::system::error_code, int) { f1 = true; }));
    ts2.insert_internal_timer(t2, system_clock::now() + 10,
        operation([&f2](lcy::system::error_code, int) { f2 = true; }));

    ::usleep(30 * 1000);

    std::list<operation> ops;
    tsq.get_ready_internal_timer_ops(ops);
    assert(ops.size() == 2);
    for ( auto& op : ops ) op();
    assert(f1 && f2);

    tsq.free(t1); tsq.free(t2);
    std::cout << "  two timer sets ok\n";
}

static void test_min_wait()
{
    std::cout << "[test_min_wait]\n";
    timer_set_queue tsq;
    basic_timer_set<steady_clock> ts1, ts2;
    tsq.add_timer_set(&ts1);
    tsq.add_timer_set(&ts2);

    auto t1 = tsq.alloc();
    auto t2 = tsq.alloc();
    ts1.insert_internal_timer(t1, steady_clock::now() + 100,
        operation([](lcy::system::error_code, int) {}));
    ts2.insert_internal_timer(t2, steady_clock::now() + 50,
        operation([](lcy::system::error_code, int) {}));

    long long w = tsq.get_wait_ms();
    assert(w > 0 && w <= 50);   // 最小

    ts1.remove_internal_timer(t1);
    ts2.remove_internal_timer(t2);
    tsq.free(t1); tsq.free(t2);
    std::cout << "  min wait ok\n";
}

static void test_remove_timer_set()
{
    std::cout << "[test_remove_timer_set]\n";
    timer_set_queue tsq;
    basic_timer_set<steady_clock> ts;
    tsq.add_timer_set(&ts);

    auto t = tsq.alloc();
    ts.insert_internal_timer(t, steady_clock::now() + 1000,
        operation([](lcy::system::error_code, int) {}));

    tsq.remove_timer_set(&ts);

    std::list<operation> ops;
    tsq.get_ready_internal_timer_ops(ops);
    assert(ops.empty());

    ts.remove_internal_timer(t);
    tsq.free(t);
    std::cout << "  remove_timer_set ok\n";
}

static void test_cleanup()
{
    std::cout << "[test_cleanup]\n";
    basic_timer_set<steady_clock> ts;
    auto t = new timer_set_base::internal_timer;
    ts.insert_internal_timer(t, steady_clock::now() + 100,
        operation([](lcy::system::error_code, int) {}));
    assert(ts.has_internal_timer(t));
    ts.cleanup_timers();
    assert(!ts.has_internal_timer(t));
    delete t;
    std::cout << "  cleanup ok\n";
}

static void test_original()
{
    std::cout << "[test_original]\n";
    timer_set_queue tsq;
    basic_timer_set<steady_clock> steady_ts;
    basic_timer_set<system_clock> system_ts;
    tsq.add_timer_set(&steady_ts);
    tsq.add_timer_set(&system_ts);

    auto t = tsq.alloc();
    bool triggered = false;
    steady_ts.insert_internal_timer(t, steady_clock::now() + 10,
        operation([&triggered](lcy::system::error_code, int) { triggered = true; }));

    assert(steady_ts.has_internal_timer(t));
    ::usleep(30 * 1000);

    std::list<operation> ops;
    tsq.get_ready_internal_timer_ops(ops);
    assert(!ops.empty());
    for ( auto& op : ops ) op();
    assert(triggered);
    assert(!steady_ts.has_internal_timer(t));

    tsq.free(t);
    std::cout << "  original ok\n";
}

int main()
{
    test_alloc_free();
    test_insert_has_remove();
    test_get_wait_ms();
    test_get_ready();
    test_not_expired();
    test_multi_timers();
    test_multi_timer_sets();
    test_min_wait();
    test_remove_timer_set();
    test_cleanup();
    test_original();

    std::cout << "\nall timer_set_queue tests passed\n";
    return 0;
}