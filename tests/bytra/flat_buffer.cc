// ./test_flat_buffer.cpp
#include <cassert>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

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

void write_bytes(lcy::bytra::flat_buffer& buf, const char* data, std::size_t n)
{
    if ( n == 0 ) { return; }
    auto view = buf.prepare(n);
    std::memcpy(view.data(), data, n);
    buf.commit(n);
}

void write_string(lcy::bytra::flat_buffer& buf, const std::string& s)
{
    write_bytes(buf, s.data(), s.size());
}

void test_initial_state()
{
    std::cout << "  case: empty buffer, size==0, capacity==0" << std::endl;

    lcy::bytra::flat_buffer buf;
    assert(buf.size() == 0);
    assert(buf.capacity() == 0);
    assert(buf.available() == 0);
    assert(drain(buf).empty());
}

void test_basic_write()
{
    std::cout << "  case: prepare/commit \"hello\"" << std::endl;

    lcy::bytra::flat_buffer buf;
    auto view = buf.prepare(5);
    assert(view.size() == 5);
    std::memcpy(view.data(), "hello", 5);
    buf.commit(5);

    assert(buf.size() == 5);
    assert(std::string(static_cast<const char*>(buf.data()), buf.size()) == "hello");
}

void test_multiple_writes()
{
    std::cout << "  case: three writes concatenate" << std::endl;

    lcy::bytra::flat_buffer buf;
    write_string(buf, "abc");
    write_string(buf, "def");
    write_string(buf, "ghi");

    assert(buf.size() == 9);
    assert(drain(buf) == "abcdefghi");
}

void test_partial_consume()
{
    std::cout << "  case: consume 3 of 9 bytes" << std::endl;

    lcy::bytra::flat_buffer buf;
    write_string(buf, "abcdefghi");

    buf.consume(3);
    assert(buf.size() == 6);
    assert(drain(buf) == "defghi");
}

void test_consume_all_resets()
{
    std::cout << "  case: consume all resets read/write index" << std::endl;

    lcy::bytra::flat_buffer buf;
    write_string(buf, "xyz");
    buf.consume(3);

    assert(buf.size() == 0);
    write_string(buf, "new");
    assert(drain(buf) == "new");
}

void test_consume_overflow_clamped()
{
    std::cout << "  case: consume(100) on size 5 clamps to 5" << std::endl;

    lcy::bytra::flat_buffer buf;
    write_string(buf, "hello");
    buf.consume(100);

    assert(buf.size() == 0);
}

void test_commit_overflow_clamped()
{
    std::cout << "  case: commit(100) after prepare(5) clamps" << std::endl;

    lcy::bytra::flat_buffer buf;
    buf.prepare(5);
    std::size_t before = buf.size();
    buf.commit(100);
    std::size_t after = buf.size();

    std::cout << "    before=" << before << " after=" << after
              << " capacity=" << buf.capacity() << std::endl;

    assert(buf.size() <= buf.capacity());
    assert(after >= 5);
}

void test_prepare_grows()
{
    std::cout << "  case: prepare grows capacity" << std::endl;

    lcy::bytra::flat_buffer buf;
    std::size_t cap0 = buf.capacity();
    buf.prepare(100);
    assert(buf.capacity() >= 100);
    assert(buf.capacity() > cap0);
}

void test_compaction()
{
    std::cout << "  case: read_index > size/2 triggers memmove" << std::endl;

    lcy::bytra::flat_buffer buf;

    std::string a(100, 'a');
    write_string(buf, a);

    buf.consume(60);
    assert(buf.size() == 40);

    auto view = buf.prepare(10);
    std::memcpy(view.data(), "0123456789", 10);
    buf.commit(10);

    assert(buf.size() == 50);
    std::string got = drain(buf);
    assert(got.size() == 50);
    assert(got.substr(0, 40) == std::string(40, 'a'));
    assert(got.substr(40) == "0123456789");
}

void test_stress()
{
    std::cout << "  case: 1000 iterations write + partial consume" << std::endl;

    lcy::bytra::flat_buffer buf;
    std::string expected;

    for ( int i = 0; i < 1000; ++i ) {
        std::string chunk = std::to_string(i) + ",";
        write_string(buf, chunk);
        expected += chunk;

        if ( i % 10 == 9 ) {
            std::size_t half = buf.size() / 2;
            std::string taken(static_cast<const char*>(buf.data()), half);
            buf.consume(half);
            expected.erase(0, half);
        }
    }

    assert(drain(buf) == expected);
}

void test_binary_safe()
{
    std::cout << "  case: raw bytes incl. \\0, \\xff" << std::endl;

    lcy::bytra::flat_buffer buf;
    const char raw[] = { 'a', '\0', '\xff', '\r', '\n', 'b' };
    write_bytes(buf, raw, sizeof(raw));

    assert(buf.size() == sizeof(raw));
    assert(std::memcmp(buf.data(), raw, sizeof(raw)) == 0);
}

void test_clear()
{
    std::cout << "  case: clear resets size to 0" << std::endl;

    lcy::bytra::flat_buffer buf;
    write_string(buf, "data");
    assert(buf.size() == 4);

    buf.clear();
    assert(buf.size() == 0);
    assert(drain(buf).empty());

    write_string(buf, "after");
    assert(drain(buf) == "after");
}

void test_const_data()
{
    std::cout << "  case: const data() readable" << std::endl;

    lcy::bytra::flat_buffer buf;
    write_string(buf, "const");

    const lcy::bytra::flat_buffer& cbuf = buf;
    std::string got(static_cast<const char*>(cbuf.data()), cbuf.size());
    assert(got == "const");
}

void test_empty_data()
{
    std::cout << "  case: data() on empty buffer is safe to call" << std::endl;

    lcy::bytra::flat_buffer buf;
    (void)buf.data();
    const lcy::bytra::flat_buffer& cbuf = buf;
    (void)cbuf.data();
    assert(buf.size() == 0);
}

void test_prepare_zero()
{
    std::cout << "  case: prepare(0) returns empty view" << std::endl;

    lcy::bytra::flat_buffer buf;
    auto view = buf.prepare(0);
    assert(view.size() == 0);
    buf.commit(0);
    assert(buf.size() == 0);
}

void test_consume_zero()
{
    std::cout << "  case: consume(0) is no-op" << std::endl;

    lcy::bytra::flat_buffer buf;
    write_string(buf, "abc");
    buf.consume(0);
    assert(buf.size() == 3);
    assert(drain(buf) == "abc");
}

} // namespace

int main()
{
    std::cout << "===== flat_buffer tests =====" << std::endl;

    RUN_TEST(test_initial_state);
    RUN_TEST(test_basic_write);
    RUN_TEST(test_multiple_writes);
    RUN_TEST(test_partial_consume);
    RUN_TEST(test_consume_all_resets);
    RUN_TEST(test_consume_overflow_clamped);
    RUN_TEST(test_commit_overflow_clamped);
    RUN_TEST(test_prepare_grows);
    RUN_TEST(test_compaction);
    RUN_TEST(test_stress);
    RUN_TEST(test_binary_safe);
    RUN_TEST(test_clear);
    RUN_TEST(test_const_data);
    RUN_TEST(test_empty_data);
    RUN_TEST(test_prepare_zero);
    RUN_TEST(test_consume_zero);

    std::cout << "===== " << g_passed << " passed, "
              << g_failed << " failed =====" << std::endl;
    return g_failed == 0 ? 0 : 1;
}