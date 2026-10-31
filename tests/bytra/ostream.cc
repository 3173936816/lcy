// ./test_ostream.cpp
#include <cassert>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <climits>

#include "lcy/bytra/ostream.hpp"
#include "lcy/bytra/flat_buffer.hpp"

namespace {

int g_passed = 0;
int g_failed = 0;

#define RUN_TEST(fn)                                            \
    do {                                                        \
        std::cout << "[ RUN      ] " << #fn << std::endl;       \
        fn();                                                   \
        std::cout << "[       OK ] " << #fn << std::endl;       \
        ++g_passed;                                             \
    } while ( 0 )

std::string drain(lcy::bytra::flat_buffer& buf)
{
    std::string s;
    if ( buf.size() > 0 ) {
        s.assign(static_cast<const char*>(buf.data()), buf.size());
    }
    buf.consume(buf.size());
    return s;
}

void test_strings()
{
    std::cout << "  case: const char* / std::string / char / empty / embedded \\0"
              << std::endl;

    lcy::bytra::flat_buffer buf;
    lcy::bytra::ostream<lcy::bytra::flat_buffer> os(buf);

    os << "hello" << ' ' << std::string("world") << '\n';
    assert(drain(buf) == "hello world\n");

    os << "" << std::string();
    assert(drain(buf) == "");

    std::string with_nul("a\0b", 3);
    os << with_nul;
    std::string got = drain(buf);
    assert(got.size() == 3);
    assert(got[0] == 'a' && got[1] == '\0' && got[2] == 'b');
}

void test_arithmetic()
{
    std::cout << "  case: int/double/bool/INT_MIN/INT_MAX" << std::endl;

    lcy::bytra::flat_buffer buf;
    lcy::bytra::ostream<lcy::bytra::flat_buffer> os(buf);

    os << 0 << ' ' << -42 << ' ' << 123456789;
    assert(drain(buf) == "0 -42 123456789");

    os << ' ' << 3.5 << ' ' << 0.0;
    assert(drain(buf) == " 3.5 0");

    os << ' ' << true << ' ' << false;
    assert(drain(buf) == " 1 0");

    os << ' ' << INT_MIN << ' ' << INT_MAX;
    std::ostringstream expect;
    expect << ' ' << INT_MIN << ' ' << INT_MAX;
    assert(drain(buf) == expect.str());
}

void test_manipulators()
{
    std::cout << "  case: hex/dec/setw/setfill/setprecision" << std::endl;

    lcy::bytra::flat_buffer buf;
    lcy::bytra::ostream<lcy::bytra::flat_buffer> os(buf);

    os << std::hex << 255 << ' ' << std::dec << 255;
    assert(drain(buf) == "ff 255");

    os << ' ' << std::setw(5) << std::setfill('0') << 42;
    assert(drain(buf) == " 00042");

    os << ' ' << std::setprecision(3) << 3.14159;
    assert(drain(buf) == " 3.14");
}

struct point { int x, y; };

std::ostream& operator<<(std::ostream& os, const point& p)
{
    return os << '(' << p.x << ',' << p.y << ')';
}

void test_custom_type()
{
    std::cout << "  case: custom operator<<(std::ostream&, point)" << std::endl;

    lcy::bytra::flat_buffer buf;
    lcy::bytra::ostream<lcy::bytra::flat_buffer> os(buf);

    os << point{1, 2} << ' ' << point{-3, 4};
    assert(drain(buf) == "(1,2) (-3,4)");
}

void test_large_write()
{
    std::cout << "  case: 10000 bytes, triggers flat_buffer compaction" << std::endl;

    lcy::bytra::flat_buffer buf;
    lcy::bytra::ostream<lcy::bytra::flat_buffer> os(buf);

    os << "prefix";
    assert(drain(buf) == "prefix");

    std::string big(10000, 'x');
    os << big;

    std::string got = drain(buf);
    assert(got.size() == big.size());
    assert(got == big);
}

void test_overflow_path()
{
    std::cout << "  case: put() and << char (overflow)" << std::endl;

    lcy::bytra::flat_buffer buf;
    lcy::bytra::ostream<lcy::bytra::flat_buffer> os(buf);

    os.put('A');
    os.put('B');
    os.put('\n');
    assert(drain(buf) == "AB\n");

    os << 'X' << 'Y';
    assert(drain(buf) == "XY");
}

void test_flush()
{
    std::cout << "  case: flush between writes" << std::endl;

    lcy::bytra::flat_buffer buf;
    lcy::bytra::ostream<lcy::bytra::flat_buffer> os(buf);

    os << "abc";
    os.flush();
    os << "def";
    os.flush();
    assert(drain(buf) == "abcdef");
}

void test_reuse()
{
    std::cout << "  case: 100x write + drain on same buffer" << std::endl;

    lcy::bytra::flat_buffer buf;
    lcy::bytra::ostream<lcy::bytra::flat_buffer> os(buf);

    for ( int i = 0; i < 100; ++i ) {
        os << i << ',';
        std::string got = drain(buf);
        assert(got == std::to_string(i) + ",");
    }
}

void test_polymorphic_use()
{
    std::cout << "  case: use via std::ostream&" << std::endl;

    lcy::bytra::flat_buffer buf;
    lcy::bytra::ostream<lcy::bytra::flat_buffer> os(buf);

    std::ostream& base = os;
    base << "poly" << 42;

    assert(drain(buf) == "poly42");
}

void test_binary_safe()
{
    std::cout << "  case: raw bytes incl. \\0, \\xff, \\r\\n" << std::endl;

    lcy::bytra::flat_buffer buf;
    lcy::bytra::ostream<lcy::bytra::flat_buffer> os(buf);

    const char raw[] = { 'a', '\0', '\xff', '\r', '\n', 'b' };
    os.write(raw, sizeof(raw));

    std::string got = drain(buf);
    assert(got.size() == sizeof(raw));
    assert(std::memcmp(got.data(), raw, sizeof(raw)) == 0);
}

void test_multiple_xsputn()
{
    std::cout << "  case: three 5000-byte strings" << std::endl;

    lcy::bytra::flat_buffer buf;
    lcy::bytra::ostream<lcy::bytra::flat_buffer> os(buf);

    std::string a(5000, 'a'), b(5000, 'b'), c(5000, 'c');
    os << a << b << c;

    std::string got = drain(buf);
    assert(got.size() == a.size() + b.size() + c.size());
    assert(got == a + b + c);
}

void test_empty_buffer()
{
    std::cout << "  case: empty buffer data/size" << std::endl;

    lcy::bytra::flat_buffer buf;
    assert(buf.size() == 0);
    assert(drain(buf).empty());
}

void test_destructor_flush()
{
    std::cout << "  case: ostream destroyed, data already in buffer" << std::endl;

    lcy::bytra::flat_buffer buf;
    {
        lcy::bytra::ostream<lcy::bytra::flat_buffer> os(buf);
        os << "dtor";
    }
    assert(drain(buf) == "dtor");
}

} // namespace

int main()
{
    std::cout << "===== ostream tests =====" << std::endl;

    RUN_TEST(test_strings);
    RUN_TEST(test_arithmetic);
    RUN_TEST(test_manipulators);
    RUN_TEST(test_custom_type);
    RUN_TEST(test_large_write);
    RUN_TEST(test_overflow_path);
    RUN_TEST(test_flush);
    RUN_TEST(test_reuse);
    RUN_TEST(test_polymorphic_use);
    RUN_TEST(test_binary_safe);
    RUN_TEST(test_multiple_xsputn);
    RUN_TEST(test_empty_buffer);
    RUN_TEST(test_destructor_flush);

    std::cout << "===== " << g_passed << " passed, "
              << g_failed << " failed =====" << std::endl;
    return g_failed == 0 ? 0 : 1;
}