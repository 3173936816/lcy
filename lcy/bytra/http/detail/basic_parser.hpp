#ifndef __LCY_BYTRA_HTTP_DETAIL_BASIC_PARSER_HPP__
#define __LCY_BYTRA_HTTP_DETAIL_BASIC_PARSER_HPP__

#include "lcy/noncopyable.hpp"

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
    private noncopyable
{
public:
    enum phase
    {
        first_line,
        field_line,
        body,
        complete,
    };

    enum result
    {
        wait_more,
        error_occurred,
        complete,
    };

public:
    typedef std::function<
        void (
            lcy::system::error_code,
            const void* data,
            std::size_t length
        )
    > parse_func_type;

public:
    basic_parser() :
        first_line_limit_(0),
        field_line_limit_(0),
        field_total_limit_(0),
        chunk_line_limit_(0),
        body_total_limit_(0),
        parse_phase_(phase::first_line),
        parsed_bytes_(0) {}

    void set_trailer_check();

    result parse(const void* data, std::size_t length, Message& message);
    void parse_first_line(const void* data, std::size_t length, parse_func_type func);
    void parse_field_line(const void* data, std::size_t length, parse_func_type func);
    void parse_chunk_line(const void* data, std::size_t length, parse_func_type func);
    void parse_body_stream(const void* data, std::size_t length, parse_func_type func);
    void parse_trailer_check(const void* data, std::size_t length, parse_func_type func);

private:
    std::size_t first_line_limit_;
    std::size_t field_line_limit_;
    std::size_t field_total_limit_;
    std::size_t chunk_line_limit_;
    std::size_t body_total_limit_;

    phase parse_phase_;
    std::size_t parsed_bytes_;
};

}   // namespace detail
}   // namespace http
}   // namespace bytra
}   // namespace lcy

#endif  // __LCY_BYTRA_HTTP_DETAIL_BASIC_PARSER_HPP__