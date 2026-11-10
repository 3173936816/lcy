#include <iostream>
#include <string>

#include "lcy/log4cc/detail/event.hpp"
#include "lcy/log4cc/detail/formatter.hpp"

using lcy::log4cc::level;
using lcy::log4cc::detail::event;
using lcy::log4cc::detail::formatter;

namespace
{

event make_event(level l,
                 const char* file,
                 std::size_t line,
                 const char* logger_name,
                 const std::string& message)
{
    event e(l, file, line, logger_name);
    e.oss_ << message;
    return e;
}

void print(const char* title, const std::string& out)
{
    std::cout << "--- " << title << " ---\n";
    std::cout << out << "\n\n";
}

}   // namespace

int main()
{
    event info = make_event(level::info, "main.cpp", 42, "root", "hello 123");
    event warn = make_event(level::warn, "net.cpp", 108, "network", "timeout");
    event error = make_event(level::error, "db.cpp", 256, "database", "connect failed");

    {
        formatter f;
        print("default pattern", f.format(info));
    }

    {
        formatter f("[%d] [%l] %m");
        print("[%d] [%l] %m", f.format(info));
    }

    {
        formatter f("%l %f:%n %c %m");
        print("%l %f:%n %c %m", f.format(warn));
    }

    {
        formatter f("[%l] [pid:%p] [tid:%t] %m");
        print("[%l] [pid:%p] [tid:%t] %m", f.format(error));
    }

    {
        formatter f("[%d] [%l] [pid:%p] [tid:%t] [%f:%n] [%c] %m");
        print("all placeholders", f.format(info));
        print("all placeholders", f.format(warn));
        print("all placeholders", f.format(error));
    }

    {
        formatter f("literal only, no placeholders");
        print("literal only", f.format(info));
    }

    {
        formatter f("100%% done: %m");
        print("percent escape", f.format(info));
    }

    {
        formatter f("%x %m");
        print("unknown placeholder", f.format(info));
    }

    {
        formatter f("end%");
        print("trailing percent", f.format(info));
    }

    {
        formatter f("");
        print("empty pattern", f.format(info));
    }

    {
        formatter f("[%l] %m");
        event empty_msg = make_event(level::info, "a.cpp", 1, "lg", "");
        print("empty message", f.format(empty_msg));
    }

    {
        formatter f("[%l] %m");
        print("before set_pattern", f.format(info));

        f.set_pattern("%m/%l");
        print("after set_pattern", f.format(info));
    }

    return 0;
}