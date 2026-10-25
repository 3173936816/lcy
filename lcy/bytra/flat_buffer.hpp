#ifndef __LCY_BYTRA_FLAT_BUFFER_HPP__
#define __LCY_BYTRA_FLAT_BUFFER_HPP__

#include <vector>
#include <cstddef>
#include <cstring>

#include "lcy/asio/buffer_view.hpp"

namespace lcy
{
namespace bytra
{

class flat_buffer
{
public:
    flat_buffer() : read_index_(0), write_index_(0) {}

    lcy::asio::mutable_buffer_view prepare(std::size_t n)
    {
        if ( read_index_ > buffer_.size() / 2 ) {
            ::memmove(buffer_.data(),
                buffer_.data() + read_index_, size());
            write_index_ -= read_index_;
            read_index_ = 0;
        }

        if ( write_index_ + n > available() ) {
            buffer_.resize(write_index_ + n);
        }

        return lcy::asio::mutable_buffer_view(
            buffer_.data() + write_index_, n);
    }

    void commit(std::size_t n)
    {
        if ( write_index_ + n > available() ) {
            n = available();
        }
        write_index_ += n;
    }

    void consume(std::size_t n)
    {
        if ( read_index_ + n > write_index_ ) {
            n = write_index_ - read_index_;
        }

        read_index_ += n;

        if ( read_index_ == write_index_ ) {
            read_index_ = 0;
            write_index_ = 0;
        }
    }

    void* data()
    {
        return buffer_.data() + read_index_;
    }

    const void* data() const
    {
        return buffer_.data() + read_index_;
    }

    std::size_t size() const
    {
        return write_index_ - read_index_;
    }

    std::size_t capacity() const
    {
        return buffer_.size();
    }

    std::size_t available() const
    {
        return buffer_.size() - write_index_;
    }

    void clear()
    {
        read_index_ = 0;
        write_index_ = 0;
    }

private:
    std::size_t read_index_;
    std::size_t write_index_;
    std::vector<char> buffer_;
};

}   // namespace bytra
}	// namespace lcy

#endif  // __LCY_BYTRA_FLAT_BUFFER_HPP__