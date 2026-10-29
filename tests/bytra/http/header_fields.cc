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

static void test_basic_insert()
{
    header_fields h;

    h.set_field("A", "1");   // A1
    h.set_field("B", "1");   // A1 B1
    h.set_field("A", "2");   // A1 A2 B1  （A 追加到 A 区间尾，保持 A 连续）

    expect_eq(dump(h), {"A:1", "A:2", "B:1"}, "basic insert order");
}

static void test_scenario_b_then_a()
{
    header_fields h;

    h.set_field("B", "1");
    h.set_field("B", "2");
    h.set_field("A", "1");

    expect_eq(dump(h), {"B:1", "B:2", "A:1"}, "B B A order");
}

static void test_get_field()
{
    header_fields h;

    h.set_field("A", "1");
    h.set_field("A", "2");
    h.set_field("B", "1");

    auto it = h.get_field("A");
    assert(it != h.field_end());
    assert(it->first == "A" && it->second == "1");

    auto miss = h.get_field("X");
    assert(miss == h.field_end());
}

static void test_count()
{
    header_fields h;

    assert(h.count("A") == 0);

    h.set_field("A", "1");
    assert(h.count("A") == 1);

    h.set_field("A", "2");
    assert(h.count("A") == 2);

    h.set_field("A", "3");
    assert(h.count("A") == 3);

    h.set_field("B", "1");
    assert(h.count("B") == 1);
    assert(h.count("A") == 3);
}

static void test_remove_field()
{
    // 删首
    {
        header_fields h;
        h.set_field("A", "1");
        h.set_field("A", "2");
        h.set_field("A", "3");

        auto it = h.get_field("A");
        h.remove_field(it);
        expect_eq(dump(h), {"A:2", "A:3"}, "remove first");
        assert(h.count("A") == 2);
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
        assert(h.count("A") == 2);
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
        assert(h.count("A") == 2);
    }

    {
        header_fields h;
        h.set_field("A", "1");
        h.set_field("B", "1");

        auto it = h.get_field("A");
        h.remove_field(it);
        expect_eq(dump(h), {"B:1"}, "remove only A");
        assert(h.count("A") == 0);
        assert(h.count("B") == 1);
    }

    {
        header_fields h;
        h.set_field("A", "1");
        h.remove_field(h.get_field("A"));
        expect_eq(dump(h), {}, "remove to empty");
        assert(h.count("A") == 0);
        assert(h.field_begin() == h.field_end());
    }
}

static void test_remove_fields()
{
    header_fields h;
    h.set_field("A", "1");
    h.set_field("A", "2");
    h.set_field("B", "1");
    h.set_field("A", "3");
    h.set_field("C", "1");

    expect_eq(dump(h), {"A:1", "A:2", "A:3", "B:1", "C:1"}, "before remove_fields");

    h.remove_fields("A");
    expect_eq(dump(h), {"B:1", "C:1"}, "after remove_fields A");
    assert(h.count("A") == 0);
    assert(h.count("B") == 1);

    h.remove_fields("X");
    expect_eq(dump(h), {"B:1", "C:1"}, "remove_fields missing");
}

static void test_remove_all_fields()
{
    header_fields h;
    h.set_field("A", "1");
    h.set_field("B", "2");
    h.set_field("A", "3");

    h.remove_all_fields();
    assert(h.field_begin() == h.field_end());
    assert(h.count("A") == 0);
    assert(h.count("B") == 0);
    assert(h.get_field("A") == h.field_end());
}

static void test_iterate_order_and_contiguous()
{
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
                assert(jt->first != "A");
            }
            break;
        }
    }
    assert(a_count == 3);
}

static void test_stress_mixed()
{
    header_fields h;

    for ( int i = 0; i < 100; ++i ) h.set_field("A", std::to_string(i));
    for ( int i = 0; i < 50; ++i )  h.set_field("B", std::to_string(i));
    for ( int i = 0; i < 10; ++i )  h.set_field("C", std::to_string(i));

    assert(h.count("A") == 100);
    assert(h.count("B") == 50);
    assert(h.count("C") == 10);

    for ( int i = 0; i < 50; ++i ) {
        auto it = h.get_field("A");
        h.remove_field(it);
    }
    assert(h.count("A") == 50);
    assert(h.count("B") == 50);
    assert(h.count("C") == 10);

    h.remove_fields("B");
    assert(h.count("B") == 0);
    assert(h.count("A") == 50);
    assert(h.count("C") == 10);

    h.remove_fields("C");
    assert(h.count("C") == 0);
    assert(h.count("A") == 50);

    int n = 0;
    for ( auto it = h.field_begin(); it != h.field_end(); ++it ) {
        assert(it->first == "A");
        ++n;
    }
    assert(n == 50);
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

    std::cout << "all header_fields tests passed\n";
    return 0;
}