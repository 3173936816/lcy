#include <functional>
#include <iostream>
#include <string>
#include <cstddef>

struct error_code
{
    error_code(int a) : value(a) {}

    int value = 0;
    operator bool() const { return value != 0; }
};

typedef std::function<void(error_code, std::size_t)> read_func_type;

static void demo(const char* tag, read_func_type f)
{
    std::cout << tag << ": ";
    f(error_code{42}, 7);
}

int main()
{
    demo("const& lambda", [](const error_code& ec, std::size_t n) {
        std::cout << "const& ec=" << ec.value << " n=" << n << "\n";
    });
}