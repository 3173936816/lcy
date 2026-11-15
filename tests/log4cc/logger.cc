#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <regex>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include "lcy/log4cc/log4cc.hpp"

using namespace lcy::log4cc;

namespace
{

int g_failed = 0;
int g_passed = 0;

#define CHECK(cond)                                                     \
    do {                                                                \
        if ( cond ) {                                                   \
            ++g_passed;                                                 \
        }                                                               \
        else {                                                          \
            ++g_failed;                                                 \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__         \
                      << " : " << #cond << "\n";                        \
        }                                                               \
    } while ( 0 )

std::string make_temp_dir()
{
    char tmpl[] = "/tmp/log4cc_test_XXXXXX";
    char* dir = ::mkdtemp(tmpl);

    if ( !dir ) {
        throw std::runtime_error("mkdtemp failed");
    }

    return dir;
}

void remove_file(const std::string& path)
{
    ::unlink(path.c_str());
}

void test_level()
{
    CHECK(level_to_string(level::trace) == "TRACE");
    CHECK(level_to_string(level::debug) == "DEBUG");
    CHECK(level_to_string(level::info)  == "INFO");
    CHECK(level_to_string(level::warn)  == "WARN");
    CHECK(level_to_string(level::error) == "ERROR");
    CHECK(level_to_string(level::fatal) == "FATAL");

    CHECK(level_to_string(static_cast<level>(99), "UNKNOWN") == "UNKNOWN");

    level l = level::info;
    CHECK(level_from_string("INFO", l) && l == level::info);
    CHECK(level_from_string("WARN", l) && l == level::warn);
    CHECK(!level_from_string("NOT_A_LEVEL", l));
}

void test_record()
{
    record::sptr_type rsptr = std::make_shared<record>(
        level::info, "main.cpp", 42, "root");

    CHECK(rsptr->get_level() == level::info);
    CHECK(rsptr->get_file_name() == "main.cpp");
    CHECK(rsptr->get_file_line() == 42);
    CHECK(rsptr->get_logger_name() == "root");
    CHECK(rsptr->get_pid() > 0);
    CHECK(rsptr->get_thread_id() > 0);
    CHECK(rsptr->get_timestamp() > 0);

    rsptr->get_oss() << "hello";
    CHECK(rsptr->get_message() == "hello");

    rsptr->get_oss() << " world";
    CHECK(rsptr->get_message() == "hello world");
}

void test_formatter()
{
    {
        formatter f;
        CHECK(f.get_pattern() ==
            "[%d{%Y-%m-%d %H:%M:%S}] [%l] [pid:%p] [tid:%t] [%f:%n] [%c] %m%E");

        record::sptr_type rsptr = std::make_shared<record>(
            level::info, "main.cpp", 42, "root");
        rsptr->get_oss() << "hello";

        std::string out = f.format(rsptr);

        std::regex re(
            R"(^\[\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\] )"
            R"(\[INFO\] )"
            R"(\[pid:\d+\] )"
            R"(\[tid:\d+\] )"
            R"(\[main\.cpp:42\] )"
            R"(\[root\] )"
            R"(hello\n$)");

        CHECK(std::regex_match(out, re));
    }

    {
        formatter f("%l %m");
        record::sptr_type rsptr = std::make_shared<record>(
            level::warn, "a.cpp", 1, "lg");
        rsptr->get_oss() << "msg";
        CHECK(f.format(rsptr) == "WARN msg");
    }

    {
        formatter f("%d{%Y/%m/%d}");
        record::sptr_type rsptr = std::make_shared<record>(
            level::info, "a.cpp", 1, "lg");
        std::string out = f.format(rsptr);
        std::regex re(R"(^\d{4}/\d{2}/\d{2}$)");
        CHECK(std::regex_match(out, re));
    }

    {
        formatter f("%d");
        record::sptr_type rsptr = std::make_shared<record>(
            level::info, "a.cpp", 1, "lg");
        std::string out = f.format(rsptr);
        std::regex re(R"(^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}$)");
        CHECK(std::regex_match(out, re));
    }

    {
        formatter f("100%% done: %m");
        record::sptr_type rsptr = std::make_shared<record>(
            level::info, "a.cpp", 1, "lg");
        rsptr->get_oss() << "ok";
        CHECK(f.format(rsptr) == "100% done: ok");
    }

    {
        formatter f("%x %m");
        record::sptr_type rsptr = std::make_shared<record>(
            level::info, "a.cpp", 1, "lg");
        rsptr->get_oss() << "ok";
        CHECK(f.format(rsptr) == "%x ok");
    }

    {
        formatter f("end%");
        record::sptr_type rsptr = std::make_shared<record>(
            level::info, "a.cpp", 1, "lg");
        CHECK(f.format(rsptr) == "end%");
    }

    {
        formatter f("");
        record::sptr_type rsptr = std::make_shared<record>(
            level::info, "a.cpp", 1, "lg");
        rsptr->get_oss() << "ignored";
        CHECK(f.format(rsptr).empty());
    }

    {
        formatter f("%l%T%m%E");
        record::sptr_type rsptr = std::make_shared<record>(
            level::info, "a.cpp", 1, "lg");
        rsptr->get_oss() << "ok";
        CHECK(f.format(rsptr) == "INFO\tok\n");
    }
}

void test_logger()
{
    logger lg("test", level::info);

    CHECK(lg.get_name() == "test");
    CHECK(lg.get_level() == level::info);
    CHECK(lg.should_log(level::info));
    CHECK(lg.should_log(level::error));
    CHECK(!lg.should_log(level::debug));

    lg.set_level(level::debug);
    CHECK(lg.should_log(level::debug));
}

void test_file_appender()
{
    std::string dir = make_temp_dir();
    std::string path = dir + "/app.log";

    {
        file_appender_st appender(path);
        formatter::sptr_type f = std::make_shared<formatter>("%l %m%E");
        appender.set_formatter(f);

        record::sptr_type r1 = std::make_shared<record>(
            level::info, "a.cpp", 1, "lg");
        r1->get_oss() << "one";
        appender.log(r1);

        record::sptr_type r2 = std::make_shared<record>(
            level::warn, "a.cpp", 2, "lg");
        r2->get_oss() << "two";
        appender.log(r2);

        appender.reload();

        record::sptr_type r3 = std::make_shared<record>(
            level::error, "a.cpp", 3, "lg");
        r3->get_oss() << "three";
        appender.log(r3);
    }

    {
        std::ifstream ifs(path);
        std::string line;

        CHECK(std::getline(ifs, line) && line == "INFO one");
        CHECK(std::getline(ifs, line) && line == "WARN two");
        CHECK(std::getline(ifs, line) && line == "ERROR three");
    }

    remove_file(path);
    ::rmdir(dir.c_str());
}

void test_rotating_file_appender()
{
    std::string dir = make_temp_dir();
    std::string path = dir + "/app.log";

    {
        rotating_file_appender_st appender(path, 20);
        formatter::sptr_type f = std::make_shared<formatter>("%m%E");
        appender.set_formatter(f);

        record::sptr_type r1 = std::make_shared<record>(
            level::info, "a.cpp", 1, "lg");
        r1->get_oss() << "aaaaaaaaaa";
        appender.log(r1);

        record::sptr_type r2 = std::make_shared<record>(
            level::info, "a.cpp", 2, "lg");
        r2->get_oss() << "bbbbbbbbbb";
        appender.log(r2);

        record::sptr_type r3 = std::make_shared<record>(
            level::info, "a.cpp", 3, "lg");
        r3->get_oss() << "cccccccccc";
        appender.log(r3);
    }

    struct stat st;

    CHECK(::stat((dir + "/app.log.1").c_str(), &st) == 0);
    CHECK(::stat((dir + "/app.log.2").c_str(), &st) == 0);
    CHECK(::stat(path.c_str(), &st) == 0);

    remove_file(dir + "/app.log.1");
    remove_file(dir + "/app.log.2");
    remove_file(path);
    ::rmdir(dir.c_str());
}

void test_logger_dispatch()
{
    std::string dir = make_temp_dir();
    std::string path = dir + "/dispatch.log";

    logger lg("dispatch", level::info);
    lg.set_formatter(std::make_shared<formatter>("%l %m%E"));

    appender_base::sptr_type appender =
        std::make_shared<file_appender_st>(path);

    lg.add_appender(appender);

    record::sptr_type r1 = std::make_shared<record>(
        level::info, "a.cpp", 1, "dispatch");
    r1->get_oss() << "one";
    lg.log(r1);

    record::sptr_type r2 = std::make_shared<record>(
        level::debug, "a.cpp", 2, "dispatch");
    r2->get_oss() << "debug-should-not-appear";
    lg.log(r2);

    record::sptr_type r3 = std::make_shared<record>(
        level::error, "a.cpp", 3, "dispatch");
    r3->get_oss() << "three";
    lg.log(r3);

    lg.flush();

    {
        std::ifstream ifs(path);
        std::string line;

        CHECK(std::getline(ifs, line) && line == "INFO one");
        CHECK(std::getline(ifs, line) && line == "ERROR three");
        CHECK(!std::getline(ifs, line));
    }

    remove_file(path);
    ::rmdir(dir.c_str());
}

}   // namespace

int main()
{
    // test_level();
    // test_record();
    // test_formatter();
    // test_logger();
    // test_file_appender();
    // test_rotating_file_appender();
    test_logger_dispatch();

    std::cout << "passed: " << g_passed
              << ", failed: " << g_failed << "\n";

    return g_failed == 0 ? 0 : 1;
}