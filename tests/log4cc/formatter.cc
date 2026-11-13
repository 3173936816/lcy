#include <cassert>
#include <iostream>
#include <memory>
#include <regex>
#include <string>

#include "lcy/log4cc/formatter.hpp"
#include "lcy/log4cc/record.hpp"

using lcy::log4cc::level;
using lcy::log4cc::record;
using lcy::log4cc::formatter;

namespace
{

typedef std::shared_ptr<record> record_sptr;
typedef formatter<record_sptr> record_formatter;

record_sptr make_record(level l,
                        const char* file,
                        std::size_t line,
                        const char* logger_name,
                        const std::string& message)
{
    auto rsptr = std::make_shared<record>(l, file, line, logger_name);
    rsptr->get_oss() << message;
    return rsptr;
}

void print(const char* title, const std::string& out)
{
    std::cout << "--- " << title << " ---\n";
    std::cout << out << "\n\n";
}

void test_default_pattern()
{
    record_formatter f;

    assert(f.get_pattern() ==
           "[%d{%Y-%m-%d %H:%M:%S}] [%l] [pid:%p] [tid:%t] [%f:%n] [%c] %m%E");

    auto rsptr = make_record(level::info, "main.cpp", 42, "root", "hello");

    std::string out = f.format(rsptr);

    std::regex re(R"(^\[\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\] )"
                  R"(\[INFO\] )"
                  R"(\[pid:\d+\] )"
                  R"(\[tid:\d+\] )"
                  R"(\[main\.cpp:42\] )"
                  R"(\[root\] )"
                  R"(hello\n$)");

    assert(std::regex_match(out, re));

    print("default pattern", out);
}

void test_custom_pattern()
{
    record_formatter f("%l %m");

    auto rsptr = make_record(level::warn, "x.cpp", 7, "lg", "msg");

    std::string out = f.format(rsptr);

    assert(out == "WARN msg");

    print("%l %m", out);
}

void test_timestamp_custom_format()
{
    record_formatter f("%d{%Y/%m/%d}");

    auto rsptr = make_record(level::info, "a.cpp", 1, "lg", "");

    std::string out = f.format(rsptr);

    std::regex re(R"(^\d{4}/\d{2}/\d{2}$)");
    assert(std::regex_match(out, re));

    print("%d{%Y/%m/%d}", out);
}

void test_timestamp_default_format()
{
    record_formatter f("%d");

    auto rsptr = make_record(level::info, "a.cpp", 1, "lg", "");

    std::string out = f.format(rsptr);

    std::regex re(R"(^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}$)");
    assert(std::regex_match(out, re));

    print("%d", out);
}

void test_literal_only()
{
    record_formatter f("hello world");

    auto rsptr = make_record(level::info, "a.cpp", 1, "lg", "");

    std::string out = f.format(rsptr);

    assert(out == "hello world");

    print("literal only", out);
}

void test_all_placeholders()
{
    record_formatter f("[%d{%H:%M:%S}] [%l] [pid:%p] [tid:%t] [%f:%n] [%c] %m%E");

    auto rsptr = make_record(level::fatal, "file.cpp", 99, "app", "boom");

    std::string out = f.format(rsptr);

    assert(out.find("[FATAL]") != std::string::npos);
    assert(out.find("[pid:") != std::string::npos);
    assert(out.find("[tid:") != std::string::npos);
    assert(out.find("[file.cpp:99]") != std::string::npos);
    assert(out.find("[app]") != std::string::npos);
    assert(out.find(" boom\n") != std::string::npos);

    print("all placeholders", out);
}

void test_tab_and_enter()
{
    record_formatter f("%l%T%m%E");

    auto rsptr = make_record(level::info, "a.cpp", 1, "lg", "ok");

    std::string out = f.format(rsptr);

    assert(out == "INFO\tok\n");

    print("tab and enter", out);
}

void test_percent_escape()
{
    record_formatter f("100%% done: %m");

    auto rsptr = make_record(level::info, "a.cpp", 1, "lg", "ok");

    std::string out = f.format(rsptr);

    assert(out == "100% done: ok");

    print("percent escape", out);
}

void test_unknown_placeholder()
{
    record_formatter f("%x %m");

    auto rsptr = make_record(level::info, "a.cpp", 1, "lg", "ok");

    std::string out = f.format(rsptr);

    assert(out == "%x ok");

    print("unknown placeholder", out);
}

void test_trailing_percent()
{
    record_formatter f("end%");

    auto rsptr = make_record(level::info, "a.cpp", 1, "lg", "");

    std::string out = f.format(rsptr);

    assert(out == "end%");

    print("trailing percent", out);
}

void test_empty_pattern()
{
    record_formatter f("");

    auto rsptr = make_record(level::info, "a.cpp", 1, "lg", "ignored");

    std::string out = f.format(rsptr);

    assert(out.empty());

    print("empty pattern", out);
}

void test_empty_message()
{
    record_formatter f("[%l] %m");

    auto rsptr = make_record(level::info, "a.cpp", 1, "lg", "");

    std::string out = f.format(rsptr);

    assert(out == "[INFO] ");

    print("empty message", out);
}

void test_repeated_format()
{
    record_formatter f("[%l] %m");

    auto r1 = make_record(level::info, "a.cpp", 1, "lg", "one");
    auto r2 = make_record(level::warn, "a.cpp", 2, "lg", "two");

    assert(f.format(r1) == "[INFO] one");
    assert(f.format(r2) == "[WARN] two");
    assert(f.format(r1) == "[INFO] one");

    print("repeated format", f.format(r1));
}

void test_default_format_output()
{
    auto rsptr = std::make_shared<record>(
        level::info, "main.cpp", 42, "root");
    rsptr->get_oss() << "hello webcc";

    record_formatter f;

    std::string out = f.format(rsptr);

    std::regex re(
        R"(^\[\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\] )"
        R"(\[INFO\] )"
        R"(\[pid:\d+\] )"
        R"(\[tid:\d+\] )"
        R"(\[main\.cpp:42\] )"
        R"(\[root\] )"
        R"(hello webcc\n$)");

    assert(std::regex_match(out, re));

    std::cout << out;
}

}   // namespace

int main()
{
    test_default_pattern();
    test_custom_pattern();
    test_timestamp_custom_format();
    test_timestamp_default_format();
    test_literal_only();
    test_all_placeholders();
    test_tab_and_enter();
    test_percent_escape();
    test_unknown_placeholder();
    test_trailing_percent();
    test_empty_pattern();
    test_empty_message();
    test_repeated_format();
    test_default_format_output();

    std::cout << "all formatter tests passed\n";

    return 0;
}