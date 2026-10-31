namespace lcy
{
namespace bytra
{
namespace http
{
namespace detail
{

stream_parser::stream_parser() :
    is_chunk_(false),
    has_trailer_(false),
    parse_phase_(parse_phase::first_line),
    parsed_bytes_(0),
    body_remaining_(0),
    first_line_limit_(0),
    field_line_limit_(0),
    field_total_limit_(0),
    chunk_payload_limit_(0),
    body_total_limit_(0) {}

void stream_parser::set_first_line_limit(std::size_t limit)
{
    first_line_limit_ = limit;
}

void stream_parser::set_field_line_limit(std::size_t limit)
{
    field_line_limit_ = limit;
}

void stream_parser::set_field_total_limit(std::size_t limit)
{
    field_total_limit_ = limit;
}

void stream_parser::set_chunk_payload_limit(std::size_t limit)
{
    chunk_payload_limit_ = limit;
}

void stream_parser::set_body_total_limit(std::size_t limit)
{
    body_total_limit_ = limit;
}

void stream_parser::set_trailer_check(bool v)
{
    has_trailer_ = v;
}

bool stream_parser::is_check_trailer() const
{
    return has_trailer_;
}

stream_parser::parse_phase stream_parser::get_parse_phase() const
{
    return parse_phase_;
}

std::size_t stream_parser::get_parse_bytes() const
{
    return parsed_bytes_;
}

void stream_parser::reset()
{
    is_chunk_ = false;
    has_trailer_ = false;
    parse_phase_ = parse_phase::first_line;
    parsed_bytes_ = 0;
    body_remaining_ = 0;
}

std::size_t stream_parser::parse_first_line(const char* data, std::size_t length, first_line_func_type func)
{
    first_line_view view = { 0 };

    if ( parse_phase_ != parse_phase::first_line ) {
        lcy::system::error_code ec =
            make_http_error_code(lcy::bytra::http::error::phase_mismatch);
        func(ec, view, parse_result::error_occurred);

        return 0;
    }

    const char* crlf = CRLF(data, length);
    if ( !crlf ) {
        if ( first_line_limit_ && length > first_line_limit_ ) {
            lcy::system::error_code ec = 
                make_http_error_code(lcy::bytra::http::error::first_line_limit);
            func(ec, view, parse_result::error_occurred);

            return 0;
        }

        lcy::system::error_code ec;
        func(ec, view, parse_result::wait_more);

        return 0;
    }

    if ( first_line_limit_ && crlf - data > first_line_limit_ ) {
        lcy::system::error_code ec = 
            make_http_error_code(lcy::bytra::http::error::first_line_limit);
        func(ec, view, parse_result::error_occurred);

        return 0;
    }
    
    if ( !split_first_line(data, crlf - data, view) ) {
         lcy::system::error_code ec = 
            make_http_error_code(error::bad_first_line);
        func(ec, view, parse_result::error_occurred);

        return 0;
    }

    lcy::system::error_code ec;
    func(ec, view, parse_result::success);

    return crlf - data + 2;
}

std::size_t stream_parser::parse_field_line(const char* data, std::size_t length, field_line_func_type func)
{

}

std::size_t stream_parser::parse_chunk(const char* data, std::size_t length, chunk_view_func_type func)
{

}

std::size_t stream_parser::parse_body(const char* data, std::size_t length, body_view_func_type func)
{

}

std::size_t stream_parser::parse_trailer(const char* data, std::size_t length, field_line_func_type func)
{

}

const char* stream_parser::CRLF(const char* data, std::size_t length) const
{
    const char* ptr = data;

    for ( std::size_t i = 0; i < length; ++i ) {
        if ( *(ptr + i) != '\r' ) {
            continue;
        }

        if ( i + 1 < length && *(ptr + i + 1) == '\n' ) {
            return ptr + i;
        }
    }

    return 0;
}

bool stream_parser::split_first_line(const char* data, std::size_t length, first_line_view& view) const
{
    std::size_t sp1 = length;
    std::size_t sp2 = length;

    for ( std::size_t i = 0; i < length; ++i ) {
        if ( data[i] == ' ' ) {
            if ( sp1 == length ) {
                sp1 = i;
            } else {
                sp2 = i;
                break;
            }
        }
    }

    if ( sp1 == length || sp2 == length ) {
        return false;
    }
    if ( sp1 == 0 || sp2 == sp1 + 1 || sp2 + 1 >= length ) {
        return false;
    }

    view.first_.ptr_ = data;
    view.first_.length_ = sp1;

    view.second_.ptr_ = data + sp1 + 1;
    view.second_.length_ = sp2 - sp1 - 1;

    view.third_.ptr_ = data + sp2 + 1;
    view.third_.length_ = length - sp2 - 1;

    return true;
}

}   // namespace detail
}   // namespace http
}   // namespace bytra
}   // namespace lcy