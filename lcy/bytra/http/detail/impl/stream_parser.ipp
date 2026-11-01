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
    chunk_state_(chunk_size_line),
    chunk_remaining_(0),
    chunk_size_(0),
    first_line_limit_(0),
    field_line_limit_(0),
    field_total_limit_(0),
    chunk_line_limit_(0),
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

void stream_parser::set_chunk_line_limit(std::size_t limit)
{
    chunk_line_limit_ = limit;
}

void stream_parser::set_chunk_payload_limit(std::size_t limit)
{
    chunk_payload_limit_ = limit;
}

void stream_parser::set_body_total_limit(std::size_t limit)
{
    body_total_limit_ = limit;
}

void stream_parser::set_body_remaining(std::size_t n)
{
    body_remaining_ = n;
}

void stream_parser::set_chunk_mode(bool v)
{
    is_chunk_ = v;
}

bool stream_parser::is_chunk_mode() const
{
    return is_chunk_;
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
    parse_phase_ = parse_phase::first_line;
    parsed_bytes_ = 0;
    body_remaining_ = 0;
    chunk_state_ = chunk_size_line;
    chunk_remaining_ = 0;
    chunk_size_ = 0;
}

std::size_t stream_parser::parse_first_line(const char* data, std::size_t length, first_line_func_type func)
{
    first_line_view view = {};

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

    std::size_t line_len = static_cast<std::size_t>(crlf - data);

    if ( first_line_limit_ && line_len > first_line_limit_ ) {
        lcy::system::error_code ec =
            make_http_error_code(lcy::bytra::http::error::first_line_limit);
        func(ec, view, parse_result::error_occurred);

        return 0;
    }

    if ( !split_first_line(data, line_len, view) ) {
        lcy::system::error_code ec =
            make_http_error_code(lcy::bytra::http::error::bad_protocol);
        func(ec, view, parse_result::error_occurred);

        return 0;
    }

    parse_phase_ = parse_phase::field_line;
    parsed_bytes_ += line_len + 2;

    lcy::system::error_code ec;
    func(ec, view, parse_result::success);

    return line_len + 2;
}

std::size_t stream_parser::parse_field_line(const char* data, std::size_t length, field_line_func_type func)
{
    field_line_view view = {};

    if ( parse_phase_ != parse_phase::field_line && parse_phase_ != parse_phase::trailer ) {
        lcy::system::error_code ec =
            make_http_error_code(lcy::bytra::http::error::phase_mismatch);
        func(ec, view, parse_result::error_occurred);

        return 0;
    }

    const char* crlf = CRLF(data, length);
    if ( !crlf ) {
        if ( field_line_limit_ && length > field_line_limit_ ) {
            lcy::system::error_code ec =
                make_http_error_code(lcy::bytra::http::error::field_line_limit);
            func(ec, view, parse_result::error_occurred);

            return 0;
        }

        lcy::system::error_code ec;
        func(ec, view, parse_result::wait_more);

        return 0;
    }

    if ( crlf == data ) {
        parsed_bytes_ += 2;

        if ( parse_phase_ == parse_phase::trailer ) {
            parse_phase_ = parse_phase::complete;
        } else {
            parse_phase_ = parse_phase::body;
        }

        lcy::system::error_code ec;
        func(ec, view, parse_result::success);

        return 2;
    }

    std::size_t line_len = static_cast<std::size_t>(crlf - data);

    if ( field_line_limit_ && line_len > field_line_limit_ ) {
        lcy::system::error_code ec =
            make_http_error_code(lcy::bytra::http::error::field_line_limit);
        func(ec, view, parse_result::error_occurred);

        return 0;
    }

    if ( !split_field_line(data, line_len, view) ) {
        lcy::system::error_code ec =
            make_http_error_code(lcy::bytra::http::error::bad_protocol);
        func(ec, view, parse_result::error_occurred);

        return 0;
    }

    if ( field_total_limit_ && parsed_bytes_ + line_len + 2 > field_total_limit_ ) {
        lcy::system::error_code ec =
            make_http_error_code(lcy::bytra::http::error::field_total_limit);
        func(ec, view, parse_result::error_occurred);

        return 0;
    }

    parsed_bytes_ += line_len + 2;

    lcy::system::error_code ec;
    func(ec, view, parse_result::success);

    return line_len + 2;
}

std::size_t stream_parser::parse_chunk(const char* data, std::size_t length, chunk_view_func_type func)
{
    chunk_view view = {};
    std::size_t offset = 0;

    if ( parse_phase_ != parse_phase::body ) {
        lcy::system::error_code ec =
            make_http_error_code(lcy::bytra::http::error::phase_mismatch);
        func(ec, view, parse_result::error_occurred);

        return 0;
    }

    for ( ;; ) {
        if ( chunk_state_ == chunk_size_line ) {
            std::size_t size = 0;
            std::size_t consumed = 0;

            if ( !parse_chunk_size(data + offset, length - offset, size, consumed) ) {
                if ( chunk_line_limit_ && length - offset > chunk_line_limit_ ) {
                    lcy::system::error_code ec =
                        make_http_error_code(lcy::bytra::http::error::bad_protocol);
                    func(ec, view, parse_result::error_occurred);

                    return offset;
                }

                lcy::system::error_code ec;
                func(ec, view, parse_result::wait_more);

                return offset;
            }

            if ( chunk_payload_limit_ && size > chunk_payload_limit_ ) {
                lcy::system::error_code ec =
                    make_http_error_code(lcy::bytra::http::error::chunk_payload_limit);
                func(ec, view, parse_result::error_occurred);

                return offset;
            }

            offset += consumed;
            chunk_size_ = size;
            chunk_remaining_ = size;

            if ( size == 0 ) {
                if ( has_trailer_ ) {
                    parse_phase_ = parse_phase::trailer;
                } else {
                    parse_phase_ = parse_phase::complete;
                }

                lcy::system::error_code ec;
                func(ec, view, parse_result::success);

                return offset;
            }

            chunk_state_ = chunk_payload;
        }
        else if ( chunk_state_ == chunk_payload ) {
            std::size_t avail = length - offset;
            if ( avail == 0 ) {
                lcy::system::error_code ec;
                func(ec, view, parse_result::wait_more);

                return offset;
            }

            std::size_t n = avail < chunk_remaining_ ? avail : chunk_remaining_;

            if ( body_total_limit_ && parsed_bytes_ + n > body_total_limit_ ) {
                lcy::system::error_code ec =
                    make_http_error_code(lcy::bytra::http::error::body_total_limit);
                func(ec, view, parse_result::error_occurred);

                return offset;
            }

            view.size_ = chunk_size_;
            view.payload_.ptr_ = data + offset;
            view.payload_.length_ = n;

            chunk_remaining_ -= n;
            offset += n;
            parsed_bytes_ += n;

            if ( chunk_remaining_ == 0 ) {
                chunk_state_ = chunk_crlf;
            }

            lcy::system::error_code ec;
            func(ec, view, parse_result::success);

            return offset;
        }
        else {
            if ( length - offset < 2 ) {
                lcy::system::error_code ec;
                func(ec, view, parse_result::wait_more);

                return offset;
            }

            if ( data[offset] != '\r' || data[offset + 1] != '\n' ) {
                lcy::system::error_code ec =
                    make_http_error_code(lcy::bytra::http::error::bad_protocol);
                func(ec, view, parse_result::error_occurred);

                return offset;
            }

            offset += 2;
            chunk_state_ = chunk_size_line;
        }
    }
}

std::size_t stream_parser::parse_body(const char* data, std::size_t length, body_view_func_type func)
{
    body_view view = {};

    if ( parse_phase_ != parse_phase::body ) {
        lcy::system::error_code ec =
            make_http_error_code(lcy::bytra::http::error::phase_mismatch);
        func(ec, view, parse_result::error_occurred);

        return 0;
    }

    if ( length == 0 ) {
        lcy::system::error_code ec;
        func(ec, view, parse_result::wait_more);

        return 0;
    }

    std::size_t n = length;

    if ( body_remaining_ > 0 && n > body_remaining_ ) {
        n = body_remaining_;
    }

    if ( body_total_limit_ && parsed_bytes_ + n > body_total_limit_ ) {
        lcy::system::error_code ec =
            make_http_error_code(lcy::bytra::http::error::body_total_limit);
        func(ec, view, parse_result::error_occurred);

        return 0;
    }

    view.body_.ptr_ = data;
    view.body_.length_ = n;

    parsed_bytes_ += n;

    if ( body_remaining_ > 0 ) {
        body_remaining_ -= n;
    }

    if ( body_remaining_ == 0 ) {
        parse_phase_ = parse_phase::complete;
    }

    lcy::system::error_code ec;
    func(ec, view, parse_result::success);

    return n;
}

std::size_t stream_parser::parse_trailer(const char* data, std::size_t length, field_line_func_type func)
{
    if ( parse_phase_ != parse_phase::trailer ) {
        field_line_view view = {};

        lcy::system::error_code ec =
            make_http_error_code(lcy::bytra::http::error::phase_mismatch);
        func(ec, view, parse_result::error_occurred);

        return 0;
    }

    return parse_field_line(data, length, func);
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

bool stream_parser::split_field_line(const char* data, std::size_t length, field_line_view& view) const
{
    std::size_t colon = length;

    for ( std::size_t i = 0; i < length; ++i ) {
        if ( data[i] == ':' ) {
            colon = i;
            break;
        }
    }

    if ( colon == length ) {
        return false;
    }

    std::size_t key_len = colon;
    while ( key_len > 0 && (data[key_len - 1] == ' ' || data[key_len - 1] == '\t') ) {
        --key_len;
    }
    if ( key_len == 0 ) {
        return false;
    }

    std::size_t value_begin = colon + 1;
    while ( value_begin < length && (data[value_begin] == ' ' || data[value_begin] == '\t') ) {
        ++value_begin;
    }

    view.key_.ptr_ = data;
    view.key_.length_ = key_len;
    view.value_.ptr_ = data + value_begin;
    view.value_.length_ = length - value_begin;

    return true;
}

bool stream_parser::parse_chunk_size(const char* data, std::size_t length, std::size_t& size, std::size_t& consumed) const
{
    std::size_t hex_len = 0;

    while ( hex_len < length && data[hex_len] != '\r' && data[hex_len] != ';' && data[hex_len] != ' ' ) {
        ++hex_len;
    }

    if ( hex_len == 0 || hex_len == length ) {
        return false;
    }

    std::size_t value = 0;

    for ( std::size_t i = 0; i < hex_len; ++i ) {
        char c = data[i];

        if ( c >= '0' && c <= '9' ) {
            value = value * 16 + (c - '0');
        } else if ( c >= 'a' && c <= 'f' ) {
            value = value * 16 + (c - 'a' + 10);
        } else if ( c >= 'A' && c <= 'F' ) {
            value = value * 16 + (c - 'A' + 10);
        } else {
            return false;
        }
    }

    const char* crlf = CRLF(data + hex_len, length - hex_len);
    if ( !crlf ) {
        return false;
    }

    consumed = hex_len + static_cast<std::size_t>(crlf - (data + hex_len)) + 2;
    size = value;

    return true;
}

}   // namespace detail
}   // namespace http
}   // namespace bytra
}   // namespace lcy