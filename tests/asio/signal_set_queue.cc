// test_signal_set_queue.cc
#include <cassert>
#include <iostream>
#include <list>
#include <signal.h>
#include <vector>

#include "lcy/asio/detail/signal_set_queue.hpp"

using namespace lcy::asio::detail;

static uint64_t make_mask(std::initializer_list<int> signums)
{
    uint64_t mask = 0;
    for ( int signum : signums ) {
        mask |= static_cast<uint64_t>(1) << (signum - 1);
    }
    return mask;
}

static void test_alloc_free()
{
    std::cout << "[test_alloc_free]\n";
    signal_set_queue ssq;

    auto a = ssq.alloc();
    auto b = ssq.alloc();
    assert(a != nullptr);
    assert(b != nullptr);
    assert(a != b);

    ssq.free(a);
    ssq.free(b);
    std::cout << "  alloc/free ok\n";
}

static void test_add_has_remove()
{
    std::cout << "[test_add_has_remove]\n";
    signal_set_queue ssq;

    auto a = ssq.alloc();
    assert(!ssq.has_internal_signal_set(a));

    bool changed = ssq.add_internal_signal_set(a, make_mask({SIGINT}),
        operation([](lcy::system::error_code, int) {}));
    assert(changed);
    assert(ssq.has_internal_signal_set(a));

    ssq.remove_internal_signal_set(a);
    assert(!ssq.has_internal_signal_set(a));

    ssq.free(a);
    std::cout << "  add/has/remove ok\n";
}

static void test_add_no_change()
{
    std::cout << "[test_add_no_change]\n";
    signal_set_queue ssq;

    auto a = ssq.alloc();
    auto b = ssq.alloc();

    bool c1 = ssq.add_internal_signal_set(a, make_mask({SIGINT}),
        operation([](lcy::system::error_code, int) {}));
    assert(c1);

    bool c2 = ssq.add_internal_signal_set(b, make_mask({SIGINT}),
        operation([](lcy::system::error_code, int) {}));
    assert(!c2);

    ssq.remove_internal_signal_set(a);
    ssq.remove_internal_signal_set(b);
    ssq.free(a);
    ssq.free(b);
    std::cout << "  add no-change ok\n";
}

static void test_get_ready_multi()
{
    std::cout << "[test_get_ready_multi]\n";
    signal_set_queue ssq;

    auto a = ssq.alloc();
    auto b = ssq.alloc();
    auto c = ssq.alloc();

    int fired = 0;
    auto cb = [&fired](lcy::system::error_code ec, int) {
        if ( !ec ) ++fired;
    };

    ssq.add_internal_signal_set(a, make_mask({SIGINT}), operation(cb));
    ssq.add_internal_signal_set(b, make_mask({SIGINT, SIGTERM}), operation(cb));
    ssq.add_internal_signal_set(c, make_mask({SIGHUP}), operation(cb));

    std::list<operation> ops;
    ssq.get_ready_internal_signal_set_ops(SIGINT, ops);

    assert(ops.size() == 2);
    for ( auto& op : ops ) op();
    assert(fired == 2);

    assert(!ssq.has_internal_signal_set(a));
    assert(!ssq.has_internal_signal_set(b));
    assert(ssq.has_internal_signal_set(c));

    ssq.remove_internal_signal_set(c);
    ssq.free(a);
    ssq.free(b);
    ssq.free(c);
    std::cout << "  get_ready multi fired=2\n";
}

static void test_get_ready_none()
{
    std::cout << "[test_get_ready_none]\n";
    signal_set_queue ssq;

    auto a = ssq.alloc();
    ssq.add_internal_signal_set(a, make_mask({SIGINT}),
        operation([](lcy::system::error_code, int) {}));

    std::list<operation> ops;
    ssq.get_ready_internal_signal_set_ops(SIGTERM, ops);
    assert(ops.empty());
    assert(ssq.has_internal_signal_set(a));

    ssq.remove_internal_signal_set(a);
    ssq.free(a);
    std::cout << "  get_ready none ok\n";
}

static void test_signal_table()
{
    std::cout << "[test_signal_table]\n";
    signal_set_queue ssq;

    auto a = ssq.alloc();
    auto b = ssq.alloc();
    ssq.add_internal_signal_set(a, make_mask({SIGINT, SIGHUP}),
        operation([](lcy::system::error_code, int) {}));
    ssq.add_internal_signal_set(b, make_mask({SIGTERM}),
        operation([](lcy::system::error_code, int) {}));

    std::list<int> signums;
    ssq.get_signal_table(signums);

    assert(signums.size() == 3);
    signums.sort();
    assert(signums.front() == SIGHUP);
    assert(signums.back() == SIGTERM);

    ssq.remove_internal_signal_set(a);
    ssq.remove_internal_signal_set(b);
    ssq.free(a);
    ssq.free(b);
    std::cout << "  signal table ok\n";
}

static void test_get_all_ops()
{
    std::cout << "[test_get_all_ops]\n";
    signal_set_queue ssq;

    auto a = ssq.alloc();
    auto b = ssq.alloc();

    int fired = 0;
    ssq.add_internal_signal_set(a, make_mask({SIGINT}),
        operation([&fired](lcy::system::error_code, int) { ++fired; }));
    ssq.add_internal_signal_set(b, make_mask({SIGTERM}),
        operation([&fired](lcy::system::error_code, int) { ++fired; }));

    std::list<operation> ops;
    ssq.get_all_signal_set_ops(ops);

    assert(ops.size() == 2);
    for ( auto& op : ops ) op();
    assert(fired == 2);

    ssq.free(a);
    ssq.free(b);
    std::cout << "  get_all ops fired=2\n";
}

static void test_full_flow()
{
    std::cout << "[test_full_flow]\n";
    signal_set_queue ssq;

    auto a = ssq.alloc();

    bool triggered = false;
    int got_signum = -1;

    ssq.add_internal_signal_set(a, make_mask({SIGINT, SIGHUP}),
        operation([&](lcy::system::error_code ec, int signum) {
            if ( !ec ) {
                triggered = true;
                got_signum = signum;
            }
        }));

    assert(ssq.has_internal_signal_set(a));

    std::list<operation> ops;
    ssq.get_ready_internal_signal_set_ops(SIGINT, ops);
    assert(ops.size() == 1);

    for ( auto& op : ops ) op();

    assert(triggered);
    assert(got_signum == SIGINT);
    assert(!ssq.has_internal_signal_set(a));

    ssq.free(a);
    std::cout << "  full flow ok, signum=" << got_signum << "\n";
}

static void test_multi_signals()
{
    std::cout << "[test_multi_signals]\n";
    signal_set_queue ssq;

    auto a = ssq.alloc();
    int count = 0;

    ssq.add_internal_signal_set(a, make_mask({SIGINT, SIGHUP}),
        operation([&count](lcy::system::error_code, int) { ++count; }));

    std::list<operation> ops1;
    ssq.get_ready_internal_signal_set_ops(SIGINT, ops1);
    for ( auto& op : ops1 ) op();
    assert(count == 1);
    assert(!ssq.has_internal_signal_set(a));

    std::list<operation> ops2;
    ssq.get_ready_internal_signal_set_ops(SIGHUP, ops2);
    assert(ops2.empty());

    ssq.free(a);
    std::cout << "  multi signals count=1\n";
}

static void test_pool_reuse()
{
    std::cout << "[test_pool_reuse]\n";
    signal_set_queue ssq;

    auto a = ssq.alloc();
    ssq.add_internal_signal_set(a, make_mask({SIGINT}),
        operation([](lcy::system::error_code, int) {}));

    std::list<operation> ops;
    ssq.get_ready_internal_signal_set_ops(SIGINT, ops);
    for ( auto& op : ops ) op();

    ssq.free(a);

    auto b = ssq.alloc();
    assert(b == a);

    bool fired = false;
    bool changed = ssq.add_internal_signal_set(b, make_mask({SIGTERM}),
        operation([&fired](lcy::system::error_code ec, int) {
            if ( !ec ) fired = true;
        }));
    assert(changed);
    assert(ssq.has_internal_signal_set(b));

    std::list<operation> ops2;
    ssq.get_ready_internal_signal_set_ops(SIGTERM, ops2);
    for ( auto& op : ops2 ) op();
    assert(fired);

    ssq.free(b);
    std::cout << "  pool reuse ok\n";
}

static void test_empty_get_ready()
{
    std::cout << "[test_empty_get_ready]\n";
    signal_set_queue ssq;

    std::list<operation> ops;
    ssq.get_ready_internal_signal_set_ops(SIGINT, ops);
    assert(ops.empty());

    std::list<int> signums;
    ssq.get_signal_table(signums);
    assert(signums.empty());

    std::cout << "  empty get_ready ok\n";
}

static void test_stress()
{
    std::cout << "[test_stress]\n";
    signal_set_queue ssq;

    const int N = 100;
    std::vector<signal_set_queue::internal_signal_set*> sets;
    int fired = 0;

    for ( int i = 0; i < N; ++i ) {
        auto s = ssq.alloc();
        uint64_t mask = (i % 10 == 0) ? make_mask({SIGINT}) : make_mask({SIGTERM});
        ssq.add_internal_signal_set(s, mask,
            operation([&fired](lcy::system::error_code, int) { ++fired; }));
        sets.push_back(s);
    }

    std::list<operation> ops;
    ssq.get_ready_internal_signal_set_ops(SIGINT, ops);
    assert(ops.size() == N / 10);
    for ( auto& op : ops ) op();
    assert(fired == N / 10);

    std::list<operation> ops2;
    ssq.get_ready_internal_signal_set_ops(SIGTERM, ops2);
    assert(ops2.size() == N - N / 10);
    for ( auto& op : ops2 ) op();
    assert(fired == N);

    for ( auto s : sets ) ssq.free(s);
    std::cout << "  stress N=" << N << " fired=" << fired << "\n";
}

int main()
{
    test_alloc_free();
    test_add_has_remove();
    test_add_no_change();
    test_get_ready_multi();
    test_get_ready_none();
    test_signal_table();
    test_get_all_ops();
    test_full_flow();
    test_multi_signals();
    test_pool_reuse();
    test_empty_get_ready();
    test_stress();

    std::cout << "\nall signal_set_queue tests passed\n";
    return 0;
}