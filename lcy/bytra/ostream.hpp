#ifndef __LCY_BYTRA_OSTREAM_HPP__
#define __LCY_BYTRA_OSTREAM_HPP__

#include <cstddef>
#include <cstring>
#include <ostream>
#include <streambuf>

namespace lcy
{
namespace bytra
{

template <typename Buffer>
class ostream :
    public std::ostream
{
private:
    class buffer_streambuf :
        public std::streambuf
    {
    public:
        explicit buffer_streambuf(Buffer& buffer) : buffer_(buffer) {}

    protected:
        std::streamsize xsputn(const char* s, std::streamsize n) override
        {
            if ( n <= 0 ) { return 0; }
            auto view = buffer_.prepare(static_cast<std::size_t>(n));
            ::memcpy(view.data(), s, static_cast<std::size_t>(n));
            buffer_.commit(static_cast<std::size_t>(n));

            return n;
        }

        int_type overflow(int_type ch) override
        {
            if ( traits_type::eq_int_type(ch, traits_type::eof()) ) {
                return traits_type::not_eof(ch);
            }
            char c = traits_type::to_char_type(ch);

            auto view = buffer_.prepare(1);
            ::memcpy(view.data(), &c, 1);
            buffer_.commit(1);

            return ch;
        }

    private:
        Buffer& buffer_;
    };

public:
    explicit ostream(Buffer& buffer) :
        std::ostream(0), streambuf_(buffer)
    {
        rdbuf(&streambuf_);
    }

private:
    buffer_streambuf streambuf_;
};

}   // namespace bytra
}   // namespace lcy

#endif  // __LCY_BYTRA_OSTREAM_HPP__