#include "lcy/bytra/http/detail/header_fields.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using lcy::bytra::http::detail::header_fields;

static std::vector<std::string> dump(const header_fields& h)
{
    std::vector<std::string> out;
    for ( auto it = h.field_begin(); it != h.field_end(); ++it ) {
        out.push_back(it->first + ":" + it->second);
    }
    return out;
}

static void expect_eq(const std::vector<std::string>& got,
                      const std::vector<std::string>& want,
                      const char* msg)
{
    if ( got != want ) {
        std::cerr << "FAIL: " << msg << "\n  got : ";
        for ( auto& s : got ) std::cerr << s << " ";
        std::cerr << "\n  want: ";
        for ( auto& s : want ) std::cerr << s << " ";
        std::cerr << "\n";
        std::abort();
    }
}

static void expect_size(std::size_t got, std::size_t want, const char* msg)
{
    if ( got != want ) {
        std::cerr << "FAIL: " << msg << "\n  got : " << got
                  << "\n  want: " << want << "\n";
        std::abort();
    }
}

static void expect_true(bool cond, const char* msg)
{
    if ( !cond ) {
        std::cerr << "FAIL: " << msg << "\n";
        std::abort();
    }
}

static void test_basic_insert()
{
    std::cout << "[test_basic_insert]\n";
    header_fields h;

    h.set_field("A", "1");
    h.set_field("B", "1");
    h.set_field("A", "2");

    expect_eq(dump(h), {"A:1", "A:2", "B:1"}, "basic insert order");
    std::cout << "  basic insert ok\n";
}

static void test_scenario_b_then_a()
{
    std::cout << "[test_scenario_b_then_a]\n";
    header_fields h;

    h.set_field("B", "1");
    h.set_field("B", "2");
    h.set_field("A", "1");

    expect_eq(dump(h), {"B:1", "B:2", "A:1"}, "B B A order");
    std::cout << "  scenario B then A ok\n";
}

static void test_get_field()
{
    std::cout << "[test_get_field]\n";
    header_fields h;

    h.set_field("A", "1");
    h.set_field("A", "2");
    h.set_field("B", "1");

    auto it = h.get_field("A");
    expect_true(it != h.field_end(), "get_field A found");
    expect_true(it->first == "A" && it->second == "1", "get_field first value");

    auto miss = h.get_field("X");
    expect_true(miss == h.field_end(), "get_field X is end");

    std::cout << "  get_field ok\n";
}

static void test_count()
{
    std::cout << "[test_count]\n";
    header_fields h;

    expect_size(h.count("A"), 0, "count A initial");
    h.set_field("A", "1");
    expect_size(h.count("A"), 1, "count A after 1");
    h.set_field("A", "2");
    expect_size(h.count("A"), 2, "count A after 2");
    h.set_field("A", "3");
    expect_size(h.count("A"), 3, "count A after 3");
    h.set_field("B", "1");
    expect_size(h.count("B"), 1, "count B");
    expect_size(h.count("A"), 3, "count A still 3");

    std::cout << "  count ok\n";
}

static void test_remove_field()
{
    std::cout << "[test_remove_field]\n";
    {
        header_fields h;
        h.set_field("A", "1");
        h.set_field("A", "2");
        h.set_field("A", "3");

        auto it = h.get_field("A");
        h.remove_field(it);
        expect_eq(dump(h), {"A:2", "A:3"}, "remove first");
        expect_size(h.count("A"), 2, "count after remove first");
    }

    {
        header_fields h;
        h.set_field("A", "1");
        h.set_field("A", "2");
        h.set_field("A", "3");

        auto it = h.get_field("A");
        ++it; ++it;
        h.remove_field(it);
        expect_eq(dump(h), {"A:1", "A:2"}, "remove last");
        expect_size(h.count("A"), 2, "count after remove last");
    }

    {
        header_fields h;
        h.set_field("A", "1");
        h.set_field("A", "2");
        h.set_field("A", "3");

        auto it = h.get_field("A");
        ++it;
        h.remove_field(it);
        expect_eq(dump(h), {"A:1", "A:3"}, "remove middle");
        expect_size(h.count("A"), 2, "count after remove middle");
    }

    {
        header_fields h;
        h.set_field("A", "1");
        h.set_field("B", "1");

        auto it = h.get_field("A");
        h.remove_field(it);
        expect_eq(dump(h), {"B:1"}, "remove only A");
        expect_size(h.count("A"), 0, "count A after remove only");
        expect_size(h.count("B"), 1, "count B after remove only");
    }

    {
        header_fields h;
        h.set_field("A", "1");
        h.remove_field(h.get_field("A"));
        expect_eq(dump(h), {}, "remove to empty");
        expect_size(h.count("A"), 0, "count A after remove to empty");
        expect_true(h.field_begin() == h.field_end(), "empty begin==end");
    }

    std::cout << "  remove_field ok\n";
}

static void test_remove_fields()
{
    std::cout << "[test_remove_fields]\n";
    header_fields h;
    h.set_field("A", "1");
    h.set_field("A", "2");
    h.set_field("B", "1");
    h.set_field("A", "3");
    h.set_field("C", "1");

    expect_eq(dump(h), {"A:1", "A:2", "A:3", "B:1", "C:1"},
              "before remove_fields");

    h.remove_fields("A");
    expect_eq(dump(h), {"B:1", "C:1"}, "after remove_fields A");
    expect_size(h.count("A"), 0, "count A after remove_fields");
    expect_size(h.count("B"), 1, "count B after remove_fields");

    h.remove_fields("X");
    expect_eq(dump(h), {"B:1", "C:1"}, "remove_fields missing");

    std::cout << "  remove_fields ok\n";
}

static void test_remove_all_fields()
{
    std::cout << "[test_remove_all_fields]\n";
    header_fields h;
    h.set_field("A", "1");
    h.set_field("B", "2");
    h.set_field("A", "3");

    h.remove_all_fields();
    expect_true(h.field_begin() == h.field_end(), "empty begin==end");
    expect_size(h.count("A"), 0, "count A after remove_all");
    expect_size(h.count("B"), 0, "count B after remove_all");
    expect_true(h.get_field("A") == h.field_end(), "get_field A is end");

    std::cout << "  remove_all_fields ok\n";
}

static void test_iterate_order_and_contiguous()
{
    std::cout << "[test_iterate_order_and_contiguous]\n";
    header_fields h;
    h.set_field("A", "1");
    h.set_field("B", "1");
    h.set_field("A", "2");
    h.set_field("C", "1");
    h.set_field("A", "3");

    expect_eq(dump(h), {"A:1", "A:2", "A:3", "B:1", "C:1"},
              "contiguous same-name");

    std::string prev;
    for ( auto it = h.field_begin(); it != h.field_end(); ++it ) {
        if ( it->first == prev ) continue;
        prev = it->first;
    }

    int a_count = 0;
    bool in_a = false;
    for ( auto it = h.field_begin(); it != h.field_end(); ++it ) {
        if ( it->first == "A" ) {
            ++a_count;
            in_a = true;
        } else if ( in_a ) {
            for ( auto jt = it; jt != h.field_end(); ++jt ) {
                expect_true(jt->first != "A", "A must be contiguous");
            }
            break;
        }
    }
    expect_size(a_count, 3, "A count contiguous");

    std::cout << "  iterate order and contiguous ok\n";
}

static void test_stress_mixed()
{
    std::cout << "[test_stress_mixed]\n";
    header_fields h;

    for ( int i = 0; i < 100; ++i ) h.set_field("A", std::to_string(i));
    for ( int i = 0; i < 50; ++i )  h.set_field("B", std::to_string(i));
    for ( int i = 0; i < 10; ++i )  h.set_field("C", std::to_string(i));

    expect_size(h.count("A"), 100, "stress count A");
    expect_size(h.count("B"), 50, "stress count B");
    expect_size(h.count("C"), 10, "stress count C");

    for ( int i = 0; i < 50; ++i ) {
        auto it = h.get_field("A");
        h.remove_field(it);
    }
    expect_size(h.count("A"), 50, "stress count A after 50 removes");
    expect_size(h.count("B"), 50, "stress count B after A removes");
    expect_size(h.count("C"), 10, "stress count C after A removes");

    h.remove_fields("B");
    expect_size(h.count("B"), 0, "stress count B after remove_fields");
    expect_size(h.count("A"), 50, "stress count A after B removes");
    expect_size(h.count("C"), 10, "stress count C after B removes");

    h.remove_fields("C");
    expect_size(h.count("C"), 0, "stress count C after remove_fields");
    expect_size(h.count("A"), 50, "stress count A after C removes");

    int n = 0;
    for ( auto it = h.field_begin(); it != h.field_end(); ++it ) {
        expect_true(it->first == "A", "stress remaining all A");
        ++n;
    }
    expect_size(n, 50, "stress final A count");

    std::cout << "  stress mixed ok\n";
}

int main()
{
    test_basic_insert();
    test_scenario_b_then_a();
    test_get_field();
    test_count();
    test_remove_field();
    test_remove_fields();
    test_remove_all_fields();
    test_iterate_order_and_contiguous();
    test_stress_mixed();

    std::cout << "\nall header_fields tests passed\n";
    return 0;
}