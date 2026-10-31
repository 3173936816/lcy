#ifndef __LCY_BYTRA_HTTP_DETAIL_STREAM_PARSER_HPP__
#define __LCY_BYTRA_HTTP_DETAIL_STREAM_PARSER_HPP__

#include "lcy/noncopyable.hpp"
#include "lcy/bytra/ostream.hpp"
#include "lcy/bytra/http/detail/error.hpp"

namespace lcy
{
namespace bytra
{
namespace http
{
namespace detail
{

class stream_parser :
    private noncopyable
{
public:
    enum parse_phase
    {
        first_line,
        field_line,
        body,
        trailer,
        complete,
    };

    enum parse_result
    {
        success,
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
    inline stream_parser();

    inline void set_first_line_limit(std::size_t limit);
    inline void set_field_line_limit(std::size_t limit);
    inline void set_field_total_limit(std::size_t limit);
    inline void set_chunk_payload_limit(std::size_t limit);
    inline void set_body_total_limit(std::size_t limit);

    inline void set_trailer_check(bool v);
    inline bool is_check_trailer() const;

    inline parse_phase get_parse_phase() const;
    inline std::size_t get_parse_bytes() const;

    inline std::size_t parse_first_line(const char* data, std::size_t length, first_line_func_type func);
    inline std::size_t parse_field_line(const char* data, std::size_t length, field_line_func_type func);
    inline std::size_t parse_chunk(const char* data, std::size_t length, chunk_view_func_type func);
    inline std::size_t parse_body(const char* data, std::size_t length, body_view_func_type func);
    inline std::size_t parse_trailer(const char* data, std::size_t length, field_line_func_type func);

private:
    inline const char* CRLF(const char* data, std::size_t length) const;
    inline bool split_first_line(const char* data, std::size_t length, first_line_view& view) const; 

private:
    bool is_chunk_;
    bool has_trailer_;
    parse_phase parse_phase_;
    std::size_t parsed_bytes_;
    std::size_t body_remaining_;

    std::size_t first_line_limit_;
    std::size_t field_line_limit_;
    std::size_t field_total_limit_;
    std::size_t chunk_payload_limit_;
    std::size_t body_total_limit_;
};

}   // namespace detail
}   // namespace http
}   // namespace bytra
}   // namespace lcy

#include "lcy/bytra/http/detail/impl/stream_parser.ipp"

#endif  // __LCY_BYTRA_HTTP_DETAIL_STREAM_PARSER_HPP__