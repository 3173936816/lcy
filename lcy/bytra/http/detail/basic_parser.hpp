#ifndef __LCY_BYTRA_HTTP_DETAIL_BASIC_PARSER_HPP__
#define __LCY_BYTRA_HTTP_DETAIL_BASIC_PARSER_HPP__

#include <cstddef>
#include <string>

#include "lcy/noncopyable.hpp"
#include "lcy/bytra/http/detail/error.hpp"

namespace lcy
{
namespace bytra
{
namespace http
{
namespace detail
{

template <typename Message>
class basic_parser :
    private lcy::noncopyable
{
public:
    typedef Message message_type;

public:
    inline basic_parser();

    inline void set_header_limit(std::size_t limit);
    inline std::size_t get_header_limit() const;

    inline void set_body_limit(std::size_t limit);
    inline std::size_t get_body_limit() const;

    inline std::size_t get_parsed_bytes() const;
    inline std::size_t get_total_parsed_bytes() const;

    inline void reset();

    inline void parse_header(const char* data,
        std::size_t size, message_type& message, lcy::system::error_code& ec);
    inline void parse_body(const char* data,
        std::size_t size, message_type& message, lcy::system::error_code& ec);
    inline void parse(const char* data,
        std::size_t size, message_type& message, lcy::system::error_code& ec);

private:
    inline void parse_first_line(message_type& message, lcy::system::error_code& ec);
    inline void parse_fields(message_type& message, lcy::system::error_code& ec);
    inline void parse_chunked_body(message_type& message, lcy::system::error_code& ec);
    inline void parse_fixed_body(message_type& message, lcy::system::error_code& ec);

    inline const char* CRLF() const;
    inline bool split_first_line(const char* data, std::size_t size, message_type& message) const;

private:
    enum class phase
    {
        header_line,
        header_fields,
        body,
        done
    };

    enum class chunk_phase
    {
        size,
        data,
        data_crlf,
        trailer,
        done
    };

    std::size_t header_limit_;
    std::size_t body_limit_;

    phase phase_;
    bool is_chunk_;
    std::size_t body_remaining_;
    std::size_t parsed_bytes_;
    std::size_t total_parsed_bytes_;

    const char* buf_;
    std::size_t buf_size_;
    std::size_t buf_pos_;

    chunk_phase chunk_phase_;
    std::size_t chunk_size_;
    std::size_t body_parsed_;
};

}   // namespace detail
}   // namespace http
}   // namespace bytra
}   // namespace lcy

#include "lcy/bytra/http/detail/impl/basic_parser.ipp"

#endif  // __LCY_BYTRA_HTTP_DETAIL_BASIC_PARSER_HPP__