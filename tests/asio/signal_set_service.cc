// test_signal_set_service.cc
#include <cassert>
#include <iostream>
#include <signal.h>
#include <unistd.h>

#include "lcy/asio/detail/scheduler.hpp"
#include "lcy/asio/detail/signal_set_service.hpp"

using namespace lcy::asio::detail;

static void test_construct_destroy()
{
    std::cout << "[test_construct_destroy]\n";
    scheduler sch(1);
    signal_set_service service(sch.get_reactor());

    signal_set_service::signal_set_impl impl;
    service.construct(impl, { SIGINT });
    assert(impl.data_ != nullptr);
    service.destroy(impl);
    assert(impl.data_ == nullptr);
    std::cout << "  construct/destroy ok\n";
}

static void test_single_fire()
{
    std::cout << "[test_single_fire]\n";
    scheduler sch(1);
    signal_set_service service(sch.get_reactor());

    signal_set_service::signal_set_impl impl;
    service.construct(impl, { SIGINT });

    bool fired = false;
    int got_signum = -1;
    service.async_wait(impl, [&](lcy::system::error_code ec, int signum) {
        if ( !ec ) { fired = true; got_signum = signum; }
    });

    ::raise(SIGINT);
    sch.run();

    assert(fired);
    assert(got_signum == SIGINT);
    service.destroy(impl);
    std::cout << "  single fire ok\n";
}

static void test_two_fire()
{
    std::cout << "[test_two_fire]\n";
    scheduler sch(1);
    signal_set_service service(sch.get_reactor());

    signal_set_service::signal_set_impl impl1, impl2;
    service.construct(impl1, { SIGINT });
    service.construct(impl2, { SIGINT });

    bool fired1 = false, fired2 = false;
    service.async_wait(impl1, [&](lcy::system::error_code ec, int s) {
        if ( !ec && s == SIGINT ) fired1 = true;
    });
    service.async_wait(impl2, [&](lcy::system::error_code ec, int s) {
        if ( !ec && s == SIGINT ) fired2 = true;
    });

    ::raise(SIGINT);
    sch.run();

    assert(fired1);
    assert(fired2);
    service.destroy(impl1);
    service.destroy(impl2);
    std::cout << "  two fire ok\n";
}

static void test_multi_signals()
{
    std::cout << "[test_multi_signals]\n";
    scheduler sch(1);
    signal_set_service service(sch.get_reactor());

    signal_set_service::signal_set_impl impl1, impl2;
    service.construct(impl1, { SIGINT });
    service.construct(impl2, { SIGTERM });

    bool fired1 = false, fired2 = false;
    service.async_wait(impl1, [&](lcy::system::error_code ec, int) {
        if ( !ec ) fired1 = true;
    });
    service.async_wait(impl2, [&](lcy::system::error_code ec, int) {
        if ( !ec ) fired2 = true;
    });

    ::raise(SIGINT);
    ::raise(SIGTERM);
    sch.run();

    assert(fired1);
    assert(fired2);

    service.destroy(impl1);
    service.destroy(impl2);
    std::cout << "  multi signals ok (both fired)\n";
}

static void test_cancel()
{
    std::cout << "[test_cancel]\n";
    scheduler sch(1);
    signal_set_service service(sch.get_reactor());

    signal_set_service::signal_set_impl impl;
    service.construct(impl, { SIGINT });

    bool cancelled = false;
    bool fired = false;
    service.async_wait(impl, [&](lcy::system::error_code ec, int) {
        if ( ec ) cancelled = true;
        else      fired = true;
    });

    service.cancel(impl);
    sch.run();

    assert(cancelled);
    assert(!fired);
    service.destroy(impl);
    std::cout << "  cancel ok\n";
}

static void test_multi_mask()
{
    std::cout << "[test_multi_mask]\n";
    scheduler sch(1);
    signal_set_service service(sch.get_reactor());

    signal_set_service::signal_set_impl impl;
    service.construct(impl, { SIGINT, SIGHUP });

    bool fired = false;
    int got = -1;
    service.async_wait(impl, [&](lcy::system::error_code ec, int signum) {
        if ( !ec ) { fired = true; got = signum; }
    });

    ::raise(SIGHUP);
    sch.run();

    assert(fired);
    assert(got == SIGHUP);
    service.destroy(impl);
    std::cout << "  multi mask ok, got SIGHUP\n";
}

static void test_destroy_without_fire()
{
    std::cout << "[test_destroy_without_fire]\n";
    scheduler sch(1);
    signal_set_service service(sch.get_reactor());

    {
        signal_set_service::signal_set_impl impl;
        service.construct(impl, { SIGINT });
        service.async_wait(impl, [](lcy::system::error_code, int) {});
        service.destroy(impl);
    }
    std::cout << "  destroy without fire ok\n";
}

static void test_invalid_signum()
{
    std::cout << "[test_invalid_signum]\n";
    scheduler sch(1);
    signal_set_service service(sch.get_reactor());

    signal_set_service::signal_set_impl impl;
    bool thrown = false;
    try {
        service.construct(impl, { 0 });
    } catch ( const std::exception& e ) {
        thrown = true;
        std::cout << "  expected throw: " << e.what() << "\n";
    }
    assert(thrown);
}

int main()
{
    test_construct_destroy();
    test_single_fire();
    test_two_fire();
    test_multi_signals();
    test_cancel();
    test_multi_mask();
    test_destroy_without_fire();
    test_invalid_signum();

    std::cout << "\nall signal_set_service tests passed\n";
    return 0;
}