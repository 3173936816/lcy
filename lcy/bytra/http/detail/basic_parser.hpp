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
    enum parse_phase
    {
        first_line,
        field_line,
        body,
        complete,
    };

    enum parse_result
    {
        normal,
        wait_more,
        error_occurred,
    };

public:
    struct data_view
    {
        const char* ptr_;
        std::size_t length_;
    };

    struct first_line_view
    {
        data_view first_;
        data_view second_;
        data_view third_;
    };

    struct field_line_view
    {
        data_view key_;
        data_view value_;
    };

    struct chunk_view
    {
        std::size_t size_;
        data_view payload_;
    };

    struct body_view
    {
        data_view body_;
    };

public:
    typedef std::function<
        void (
            lcy::system::error_code ec,
            const first_line_view& view,
            parse_result result
        )
    > first_line_func_type;

    typedef std::function<
        void (
            lcy::system::error_code ec,
            const field_line_view& view,
            parse_result result
        )
    > field_line_func_type;

    typedef std::function<
        void (
            lcy::system::error_code ec,
            const chunk_view& view,
            parse_result result
        )
    > chunk_view_func_type;

    typedef std::function<
        void (
            lcy::system::error_code ec,
            const body_view& view,
            parse_result result
        )
    > body_view_func_type;

public:
    basic_parser() :
        first_line_limit_(0),
        field_line_limit_(0),
        field_total_limit_(0),
        chunk_line_limit_(0),
        body_total_limit_(0),
        has_trailer_(false),
        parse_phase_(parse_phase::first_line),
        parsed_bytes_(0) {}

    void set_trailer_check(bool v)
    {
        has_trailer_ = v;
    }

    bool is_check_trailer() const
    {
        return has_trailer_;
    }

    parse_result parse(const void* data, std::size_t length, Message& message);
    void parse_first_line(const void* data, std::size_t length, first_line_func_type func);
    void parse_field_line(const void* data, std::size_t length, field_line_func_type func);
    void parse_chunk(const void* data, std::size_t length, chunk_view_func_type func);
    void parse_body(const void* data, std::size_t length, body_view_func_type func);
    void parse_trailer(const void* data, std::size_t length, field_line_func_type func);

private:
    std::size_t first_line_limit_;
    std::size_t field_line_limit_;
    std::size_t field_total_limit_;
    std::size_t chunk_line_limit_;
    std::size_t body_total_limit_;

    bool has_trailer_;
    parse_phase parse_phase_;
    std::size_t parsed_bytes_;
};

}   // namespace detail
}   // namespace http
}   // namespace bytra
}   // namespace lcy

#endif  // __LCY_BYTRA_HTTP_DETAIL_BASIC_PARSER_HPP__