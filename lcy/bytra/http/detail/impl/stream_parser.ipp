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
    body_length_known_(false),
    chunk_state_(chunk_size_line),
    chunk_size_(0),
    chunk_remaining_(0),
    chunk_first_piece_(false),
    field_total_(0),
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
    body_length_known_ = true;
}

void stream_parser::set_body_unknown()
{
    body_remaining_ = 0;
    body_length_known_ = false;
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
    body_length_known_ = false;
    chunk_state_ = chunk_size_line;
    chunk_size_ = 0;
    chunk_remaining_ = 0;
    chunk_first_piece_ = false;
    field_total_ = 0;
}

stream_parser::scan_status stream_parser::scan_line(
    const char* data,
    std::size_t length,
    std::size_t offset,
    std::size_t max_line,
    const char*& line,
    std::size_t& line_len,
    std::size_t& consumed) const
{
    for ( std::size_t i = offset; i + 1 < length; ++i ) {
        if ( data[i] != '\r' ) {
            continue;
        }

        if ( data[i + 1] != '\n' ) {
            continue;
        }

        line_len = i - offset;

        if ( max_line && line_len > max_line ) {
            return scan_invalid;
        }

        line = data + offset;
        consumed = line_len + 2;

        return scan_complete;
    }

    if ( max_line && (length - offset) > max_line ) {
        return scan_invalid;
    }

    return scan_need_more;
}

stream_parser::scan_status stream_parser::scan_chunk_size(
    const char* data,
    std::size_t length,
    std::size_t offset,
    std::size_t max_line,
    std::size_t& chunk_size,
    std::size_t& consumed) const
{
    std::size_t i = offset;
    std::size_t value = 0;
    std::size_t hex_digits = 0;

    for ( ; i < length; ++i ) {
        char c = data[i];
        int digit = 0;

        if ( c >= '0' && c <= '9' ) {
            digit = c - '0';
        } else if ( c >= 'a' && c <= 'f' ) {
            digit = c - 'a' + 10;
        } else if ( c >= 'A' && c <= 'F' ) {
            digit = c - 'A' + 10;
        } else {
            break;
        }

        if ( value > (std::numeric_limits<std::size_t>::max() -
                      static_cast<std::size_t>(digit)) / 16 ) {
            return scan_invalid;
        }

        value = value * 16 + static_cast<std::size_t>(digit);
        ++hex_digits;
    }

    if ( hex_digits == 0 ) {
        if ( length == offset ) {
            return scan_need_more;
        }

        return scan_invalid;
    }

    for ( ; i + 1 < length; ++i ) {
        if ( data[i] != '\r' ) {
            continue;
        }

        if ( data[i + 1] != '\n' ) {
            continue;
        }

        std::size_t line_len = i - offset;

        if ( max_line && line_len > max_line ) {
            return scan_invalid;
        }

        chunk_size = value;
        consumed = line_len + 2;

        return scan_complete;
    }

    if ( max_line && (length - offset) > max_line ) {
        return scan_invalid;
    }

    return scan_need_more;
}

bool stream_parser::split_first_line(
    const char* line,
    std::size_t length,
    first_line_view& view) const
{
    std::size_t sp1 = length;
    std::size_t sp2 = length;

    for ( std::size_t i = 0; i < length; ++i ) {
        if ( line[i] == ' ' ) {
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

    view.first_.ptr_ = line;
    view.first_.length_ = sp1;

    view.second_.ptr_ = line + sp1 + 1;
    view.second_.length_ = sp2 - sp1 - 1;

    view.third_.ptr_ = line + sp2 + 1;
    view.third_.length_ = length - sp2 - 1;

    return true;
}

bool stream_parser::split_field_line(
    const char* line,
    std::size_t length,
    field_line_view& view) const
{
    std::size_t colon = length;

    for ( std::size_t i = 0; i < length; ++i ) {
        if ( line[i] == ':' ) {
            colon = i;
            break;
        }
    }

    if ( colon == length ) {
        return false;
    }

    std::size_t key_len = colon;
    while ( key_len > 0 && (line[key_len - 1] == ' ' || line[key_len - 1] == '\t') ) {
        --key_len;
    }
    if ( key_len == 0 ) {
        return false;
    }

    std::size_t value_begin = colon + 1;
    while ( value_begin < length && (line[value_begin] == ' ' || line[value_begin] == '\t') ) {
        ++value_begin;
    }

    view.key_.ptr_ = line;
    view.key_.length_ = key_len;

    view.value_.ptr_ = line + value_begin;
    view.value_.length_ = length - value_begin;

    return true;
}

std::size_t stream_parser::parse_first_line(
    const char* data,
    std::size_t length,
    first_line_func_type func)
{
    first_line_view view = {};

    if ( parse_phase_ != parse_phase::first_line ) {
        lcy::system::error_code ec =
            make_http_error_code(lcy::bytra::http::error::phase_mismatch);
        func(ec, view, parse_result::error_occurred);

        return 0;
    }

    const char* line = 0;
    std::size_t line_len = 0;
    std::size_t consumed = 0;

    scan_status st = scan_line(data, length, 0, first_line_limit_,
        line, line_len, consumed);

    if ( st == scan_need_more ) {
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

    if ( st == scan_invalid ) {
        lcy::system::error_code ec =
            make_http_error_code(lcy::bytra::http::error::first_line_limit);
        func(ec, view, parse_result::error_occurred);

        return 0;
    }

    if ( !split_first_line(line, line_len, view) ) {
        lcy::system::error_code ec =
            make_http_error_code(lcy::bytra::http::error::bad_protocol);
        func(ec, view, parse_result::error_occurred);

        return 0;
    }

    parsed_bytes_ += consumed;
    parse_phase_ = parse_phase::field_line;

    lcy::system::error_code ec;
    func(ec, view, parse_result::success);

    return consumed;
}

std::size_t stream_parser::parse_field_line(
    const char* data,
    std::size_t length,
    field_line_func_type func)
{
    field_line_view view = {};

    if ( parse_phase_ != parse_phase::field_line && parse_phase_ != parse_phase::trailer ) {
        lcy::system::error_code ec =
            make_http_error_code(lcy::bytra::http::error::phase_mismatch);
        func(ec, view, parse_result::error_occurred);

        return 0;
    }

    const char* line = 0;
    std::size_t line_len = 0;
    std::size_t consumed = 0;

    scan_status st = scan_line(data, length, 0, field_line_limit_,
        line, line_len, consumed);

    if ( st == scan_need_more ) {
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

    if ( st == scan_invalid ) {
        lcy::system::error_code ec =
            make_http_error_code(lcy::bytra::http::error::field_line_limit);
        func(ec, view, parse_result::error_occurred);

        return 0;
    }

    if ( line_len == 0 ) {
        parsed_bytes_ += consumed;

        if ( parse_phase_ == parse_phase::trailer ) {
            parse_phase_ = parse_phase::complete;
        } else {
            parse_phase_ = parse_phase::body;

            if ( !is_chunk_ && !body_length_known_ ) {
                body_remaining_ = 0;
                body_length_known_ = true;
            }
        }

        lcy::system::error_code ec;
        func(ec, view, parse_result::success);

        return consumed;
    }

    if ( !split_field_line(line, line_len, view) ) {
        lcy::system::error_code ec =
            make_http_error_code(lcy::bytra::http::error::bad_protocol);
        func(ec, view, parse_result::error_occurred);

        return 0;
    }

    if ( parse_phase_ == parse_phase::field_line ) {
        field_total_ += consumed;

        if ( field_total_limit_ && field_total_ > field_total_limit_ ) {
            lcy::system::error_code ec =
                make_http_error_code(lcy::bytra::http::error::field_total_limit);
            func(ec, view, parse_result::error_occurred);

            return 0;
        }
    }

    parsed_bytes_ += consumed;

    lcy::system::error_code ec;
    func(ec, view, parse_result::success);

    return consumed;
}

std::size_t stream_parser::parse_body(
    const char* data,
    std::size_t length,
    body_view_func_type func)
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

    if ( body_length_known_ && body_remaining_ > 0 && n > body_remaining_ ) {
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

    if ( body_length_known_ ) {
        if ( body_remaining_ >= n ) {
            body_remaining_ -= n;
        } else {
            body_remaining_ = 0;
        }

        if ( body_remaining_ == 0 ) {
            parse_phase_ = parse_phase::complete;
        }
    }

    lcy::system::error_code ec;
    func(ec, view, parse_result::success);

    return n;
}

std::size_t stream_parser::parse_chunk(
    const char* data,
    std::size_t length,
    chunk_view_func_type func)
{
    chunk_view view = {};

    if ( parse_phase_ != parse_phase::body || !is_chunk_ ) {
        lcy::system::error_code ec =
            make_http_error_code(lcy::bytra::http::error::phase_mismatch);
        func(ec, view, parse_result::error_occurred);

        return 0;
    }

    std::size_t offset = 0;

    for ( ;; ) {
        if ( chunk_state_ == chunk_size_line ) {
            std::size_t size = 0;
            std::size_t consumed = 0;

            scan_status st = scan_chunk_size(data, length, offset,
                chunk_line_limit_, size, consumed);

            if ( st == scan_need_more ) {
                lcy::system::error_code ec;
                func(ec, view, parse_result::wait_more);

                return offset;
            }

            if ( st == scan_invalid ) {
                lcy::system::error_code ec =
                    make_http_error_code(lcy::bytra::http::error::bad_protocol);
                func(ec, view, parse_result::error_occurred);

                return offset;
            }

            if ( chunk_payload_limit_ && size > chunk_payload_limit_ ) {
                lcy::system::error_code ec =
                    make_http_error_code(lcy::bytra::http::error::chunk_payload_limit);
                func(ec, view, parse_result::error_occurred);

                return offset;
            }

            offset += consumed;
            parsed_bytes_ += consumed;
            chunk_size_ = size;
            chunk_remaining_ = size;
            chunk_first_piece_ = true;

            if ( size == 0 ) {
                chunk_state_ = chunk_last_crlf;
                continue;
            }

            chunk_state_ = chunk_payload;
            continue;
        }

        if ( chunk_state_ == chunk_payload ) {
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
            view.first_piece_ = chunk_first_piece_;
            view.last_piece_ = (chunk_remaining_ == n);

            chunk_first_piece_ = false;
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

        if ( chunk_state_ == chunk_crlf ) {
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
            parsed_bytes_ += 2;
            chunk_state_ = chunk_size_line;
            continue;
        }

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
        parsed_bytes_ += 2;

        if ( has_trailer_ ) {
            parse_phase_ = parse_phase::trailer;
        } else {
            parse_phase_ = parse_phase::complete;
        }

        lcy::system::error_code ec;
        func(ec, view, parse_result::success);

        return offset;
    }
}

std::size_t stream_parser::parse_trailer(
    const char* data,
    std::size_t length,
    field_line_func_type func)
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

}   // namespace detail
}   // namespace http
}   // namespace bytra
}   // namespace lcy