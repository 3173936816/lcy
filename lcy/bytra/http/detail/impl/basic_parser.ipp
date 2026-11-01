#include <algorithm>
#include <cstddef>
#include <cstring>
#include <string>

#include "lcy/bytra/flat_buffer.hpp"
#include "lcy/bytra/http/method.hpp"
#include "lcy/bytra/http/status.hpp"
#include "lcy/bytra/http/version.hpp"

namespace lcy
{
namespace bytra
{
namespace http
{
namespace detail
{

namespace
{

inline char ascii_lower(char c)
{
    return ( c >= 'A' && c <= 'Z' ) ? static_cast<char>(c - 'A' + 'a') : c;
}

inline bool iequals(const char* b, const char* e, const char* lit)
{
    const char* p = b;
    const char* q = lit;
    for ( ; p != e && *q; ++p, ++q ) {
        if ( ascii_lower(*p) != ascii_lower(*q) ) {
            return false;
        }
    }
    return p == e && *q == '\0';
}

inline bool iequals(const std::string& s, const char* lit)
{
    return iequals(s.data(), s.data() + s.size(), lit);
}

inline bool token_list_contains(const std::string& value, const char* token)
{
    const char* p = value.data();
    const char* const end = p + value.size();
    while ( p < end ) {
        while ( p < end && ( *p == ' ' || *p == '\t' || *p == ',' ) ) {
            ++p;
        }
        const char* t = p;
        while ( p < end && *p != ',' ) {
            ++p;
        }
        const char* te = p;
        while ( te > t && ( te[-1] == ' ' || te[-1] == '\t' ) ) {
            --te;
        }
        if ( iequals(t, te, token) ) {
            return true;
        }
    }
    return false;
}

inline bool parse_uint(const char* b, const char* e, int& out)
{
    if ( b == e ) {
        return false;
    }
    int v = 0;
    for ( const char* p = b; p != e; ++p ) {
        if ( *p < '0' || *p > '9' ) {
            return false;
        }
        int d = *p - '0';
        if ( v > ( 2147483647 - d ) / 10 ) {
            return false;
        }
        v = v * 10 + d;
    }
    out = v;
    return true;
}

inline bool parse_size(const char* b, const char* e, std::size_t& out)
{
    if ( b == e ) {
        return false;
    }
    std::size_t v = 0;
    for ( const char* p = b; p != e; ++p ) {
        if ( *p < '0' || *p > '9' ) {
            return false;
        }
        std::size_t d = static_cast<std::size_t>(*p - '0');
        if ( v > (static_cast<std::size_t>(-1) - d) / 10 ) {
            return false;
        }
        v = v * 10 + d;
    }
    out = v;
    return true;
}

inline bool parse_hex(const char* b, const char* e, std::size_t& out)
{
    if ( b == e ) {
        return false;
    }
    std::size_t v = 0;
    for ( const char* p = b; p != e; ++p ) {
        int d;
        if ( *p >= '0' && *p <= '9' ) {
            d = *p - '0';
        } else if ( *p >= 'a' && *p <= 'f' ) {
            d = *p - 'a' + 10;
        } else if ( *p >= 'A' && *p <= 'F' ) {
            d = *p - 'A' + 10;
        } else {
            return false;
        }
        if ( v > (static_cast<std::size_t>(-1) - static_cast<std::size_t>(d)) / 16 ) {
            return false;
        }
        v = v * 16 + static_cast<std::size_t>(d);
    }
    out = v;
    return true;
}

inline bool parse_field_line(const char* line_begin, const char* line_end,
        std::string& name, std::string& value)
{
    const char* colon = std::find(line_begin, line_end, ':');
    if ( colon == line_end || colon == line_begin ) {
        return false;
    }
    const char* v = colon + 1;
    const char* ve = line_end;
    while ( v < ve && ( *v == ' ' || *v == '\t' ) ) {
        ++v;
    }
    while ( ve > v && ( ve[-1] == ' ' || ve[-1] == '\t' ) ) {
        --ve;
    }
    name.assign(line_begin, colon);
    value.assign(v, ve);
    return true;
}

inline bool parse_version(const char* b, const char* e, version& out)
{
    const char* const end = e;
    if ( static_cast<std::size_t>(end - b) < 8 ) {
        return false;
    }
    if ( std::string(b, b + 5) != "HTTP/" ) {
        return false;
    }
    const char* dot = std::find(b + 5, end, '.');
    if ( dot == end ) {
        return false;
    }
    int major = 0;
    int minor = 0;
    if ( !parse_uint(b + 5, dot, major) || !parse_uint(dot + 1, end, minor) ) {
        return false;
    }
    if ( major == 1 && minor == 0 ) {
        out = version::http_1_0;
        return true;
    }
    if ( major == 1 && minor == 1 ) {
        out = version::http_1_1;
        return true;
    }
    return false;
}

inline void append_body(flat_buffer& body, const char* data, std::size_t size)
{
    if ( size == 0 ) {
        return;
    }
    auto view = body.prepare(size);
    ::memcpy(view.data(), data, size);
    body.commit(size);
}

} // namespace

template <typename Message>
basic_parser<Message>::basic_parser() :
    header_limit_(8192),
    body_limit_(65536),
    phase_(phase::header_line),
    is_chunk_(false),
    body_remaining_(0),
    parsed_bytes_(0),
    total_parsed_bytes_(0),
    buf_(0),
    buf_size_(0),
    buf_pos_(0),
    chunk_phase_(chunk_phase::size),
    chunk_size_(0),
    body_parsed_(0) {}

template <typename Message>
void basic_parser<Message>::set_header_limit(std::size_t limit)
{
    header_limit_ = limit;
}

template <typename Message>
std::size_t basic_parser<Message>::get_header_limit() const
{
    return header_limit_;
}

template <typename Message>
void basic_parser<Message>::set_body_limit(std::size_t limit)
{
    body_limit_ = limit;
}

template <typename Message>
std::size_t basic_parser<Message>::get_body_limit() const
{
    return body_limit_;
}

template <typename Message>
std::size_t basic_parser<Message>::get_parsed_bytes() const
{
    return parsed_bytes_;
}

template <typename Message>
std::size_t basic_parser<Message>::get_total_parsed_bytes() const
{
    return total_parsed_bytes_;
}

template <typename Message>
void basic_parser<Message>::reset()
{
    phase_ = phase::header_line;
    is_chunk_ = false;
    body_remaining_ = 0;
    parsed_bytes_ = 0;
    total_parsed_bytes_ = 0;

    buf_ = 0;
    buf_size_ = 0;
    buf_pos_ = 0;

    chunk_phase_ = chunk_phase::size;
    chunk_size_ = 0;
    body_parsed_ = 0;
}

template <typename Message>
void basic_parser<Message>::parse_header(const char* data,
    std::size_t size, message_type& message, lcy::system::error_code& ec)
{
    if ( phase_ != phase::header_line && phase_ != phase::header_fields ) {
        ec = make_http_error_code(lcy::bytra::http::error::invalid_state);
        return;
    }

    buf_ = data;
    buf_size_ = size;
    buf_pos_ = 0;

    if ( phase_ == phase::header_line ) {
        parse_first_line(message, ec);
    } else {
        parse_fields(message, ec);
    }
}

template <typename Message>
void basic_parser<Message>::parse_body(const char* data,
    std::size_t size, message_type& message, lcy::system::error_code& ec)
{
    if ( phase_ != phase::body ) {
        ec = make_http_error_code(lcy::bytra::http::error::invalid_state);
        return;
    }

    buf_ = data;
    buf_size_ = size;
    buf_pos_ = 0;

    if ( is_chunk_ ) {
        parse_chunked_body(message, ec);
    } else {
        parse_fixed_body(message, ec);
    }
}

template <typename Message>
void basic_parser<Message>::parse(const char* data,
    std::size_t size, message_type& message, lcy::system::error_code& ec)
{
    buf_ = data;
    buf_size_ = size;
    buf_pos_ = 0;

    while ( true ) {
        if ( phase_ == phase::header_line ) {
            parse_first_line(message, ec);
        } else if ( phase_ == phase::header_fields ) {
            parse_fields(message, ec);
        } else if ( phase_ == phase::body ) {
            if ( is_chunk_ ) {
                parse_chunked_body(message, ec);
            } else {
                parse_fixed_body(message, ec);
            }
        } else {
            ec = make_http_error_code(lcy::bytra::http::error::success);
            return;
        }

        if ( ec ) {
            return;
        }
    }
}

template <typename Message>
void basic_parser<Message>::parse_first_line(message_type& message,
        lcy::system::error_code& ec)
{
    const char* crlf = CRLF();
    if ( !crlf ) {
        if ( header_limit_ && buf_size_ > header_limit_ ) {
            ec = make_http_error_code(lcy::bytra::http::error::header_too_large);
            return;
        }
        ec = make_http_error_code(lcy::bytra::http::error::wait_more);
        return;
    }

    const char* start = buf_ + buf_pos_;
    const std::size_t line_len = static_cast<std::size_t>(crlf - start);
    if ( !split_first_line(start, line_len, message) ) {
        ec = make_http_error_code(lcy::bytra::http::error::invalid_header);
        return;
    }

    buf_pos_ += line_len + 2;
    parsed_bytes_ += line_len + 2;
    total_parsed_bytes_ += line_len + 2;

    phase_ = phase::header_fields;
    ec = make_http_error_code(lcy::bytra::http::error::success);
}

template <typename Message>
void basic_parser<Message>::parse_fields(message_type& message,
        lcy::system::error_code& ec)
{
    while ( buf_pos_ < buf_size_ ) {
        const char* start = buf_ + buf_pos_;
        const char* crlf = CRLF();
        if ( !crlf ) {
            if ( header_limit_ && buf_size_ > header_limit_ ) {
                ec = make_http_error_code(lcy::bytra::http::error::header_too_large);
                return;
            }
            ec = make_http_error_code(lcy::bytra::http::error::wait_more);
            return;
        }

        const std::size_t line_len = static_cast<std::size_t>(crlf - start);
        const std::size_t consumed = line_len + 2;

        if ( line_len == 0 ) {
            buf_pos_ += consumed;
            parsed_bytes_ += consumed;
            total_parsed_bytes_ += consumed;

            std::string te;
            bool chunked = false;
            auto te_iter = message.get_field("Transfer-Encoding");
            if ( te_iter != message.field_end() ) {
                chunked = token_list_contains(te_iter->second, "chunked");
            }

            std::size_t content_length = 0;
            if ( !chunked ) {
                auto cl_iter = message.get_field("Content-Length");
                if ( cl_iter != message.field_end() ) {
                    const std::string& cl = cl_iter->second;
                    if ( !parse_size(cl.data(), cl.data() + cl.size(), content_length) ) {
                        ec = make_http_error_code(lcy::bytra::http::error::invalid_header);
                        return;
                    }
                }
            }

            if ( chunked ) {
                is_chunk_ = true;
                phase_ = phase::body;
            } else if ( content_length > 0 ) {
                is_chunk_ = false;
                body_remaining_ = content_length;
                if ( body_limit_ && body_remaining_ > body_limit_ ) {
                    ec = make_http_error_code(lcy::bytra::http::error::body_too_large);
                    return;
                }
                phase_ = phase::body;
            } else {
                phase_ = phase::done;
            }

            ec = make_http_error_code(lcy::bytra::http::error::success);
            return;
        }

        std::string name;
        std::string value;
        if ( !parse_field_line(start, crlf, name, value) ) {
            ec = make_http_error_code(lcy::bytra::http::error::invalid_header);
            return;
        }
        message.set_field(std::move(name), std::move(value));

        buf_pos_ += consumed;
        parsed_bytes_ += consumed;
        total_parsed_bytes_ += consumed;
    }

    ec = make_http_error_code(lcy::bytra::http::error::wait_more);
}

template <typename Message>
void basic_parser<Message>::parse_chunked_body(message_type& message,
        lcy::system::error_code& ec)
{
    while ( true ) {
        switch ( chunk_phase_ ) {
            case chunk_phase::size : {
                const char* start = buf_ + buf_pos_;
                const char* crlf = CRLF();
                if ( !crlf ) {
                    ec = make_http_error_code(lcy::bytra::http::error::wait_more);
                    return;
                }

                const char* semi = std::find(start, crlf, ';');
                const char* hex_end = ( semi == crlf ) ? crlf : semi;

                std::size_t sz = 0;
                if ( !parse_hex(start, hex_end, sz) ) {
                    ec = make_http_error_code(lcy::bytra::http::error::invalid_body);
                    return;
                }

                const std::size_t consumed = static_cast<std::size_t>(crlf - start) + 2;
                buf_pos_ += consumed;
                parsed_bytes_ += consumed;
                total_parsed_bytes_ += consumed;

                if ( sz == 0 ) {
                    chunk_phase_ = chunk_phase::trailer;
                } else {
                    if ( body_limit_ && body_parsed_ + sz > body_limit_ ) {
                        ec = make_http_error_code(lcy::bytra::http::error::body_too_large);
                        return;
                    }
                    chunk_size_ = sz;
                    chunk_phase_ = chunk_phase::data;
                }
                break;
            }

            case chunk_phase::data : {
                const std::size_t avail = buf_size_ - buf_pos_;
                if ( avail == 0 ) {
                    ec = make_http_error_code(lcy::bytra::http::error::wait_more);
                    return;
                }
                std::size_t take = avail;
                if ( take > chunk_size_ ) {
                    take = chunk_size_;
                }
                append_body(message.body_container(), buf_ + buf_pos_, take);
                buf_pos_ += take;
                chunk_size_ -= take;
                body_parsed_ += take;
                parsed_bytes_ += take;
                total_parsed_bytes_ += take;
                if ( chunk_size_ == 0 ) {
                    chunk_phase_ = chunk_phase::data_crlf;
                }
                break;
            }

            case chunk_phase::data_crlf : {
                const std::size_t avail = buf_size_ - buf_pos_;
                if ( avail < 2 ) {
                    ec = make_http_error_code(lcy::bytra::http::error::wait_more);
                    return;
                }
                const char* p = buf_ + buf_pos_;
                if ( p[0] != '\r' || p[1] != '\n' ) {
                    ec = make_http_error_code(lcy::bytra::http::error::invalid_body);
                    return;
                }
                buf_pos_ += 2;
                parsed_bytes_ += 2;
                total_parsed_bytes_ += 2;
                chunk_phase_ = chunk_phase::size;
                break;
            }

            case chunk_phase::trailer : {
                const char* start = buf_ + buf_pos_;
                const char* crlf = CRLF();
                if ( !crlf ) {
                    ec = make_http_error_code(lcy::bytra::http::error::wait_more);
                    return;
                }

                const std::size_t line_len = static_cast<std::size_t>(crlf - start);
                const std::size_t consumed = line_len + 2;

                if ( line_len == 0 ) {
                    buf_pos_ += consumed;
                    parsed_bytes_ += consumed;
                    total_parsed_bytes_ += consumed;
                    chunk_phase_ = chunk_phase::done;
                    phase_ = phase::done;
                    ec = make_http_error_code(lcy::bytra::http::error::success);
                    return;
                }

                std::string name;
                std::string value;
                if ( !parse_field_line(start, crlf, name, value) ) {
                    ec = make_http_error_code(lcy::bytra::http::error::invalid_body);
                    return;
                }
                message.set_field(std::move(name), std::move(value));

                buf_pos_ += consumed;
                parsed_bytes_ += consumed;
                total_parsed_bytes_ += consumed;
                break;
            }

            case chunk_phase::done: {
                phase_ = phase::done;
                ec = make_http_error_code(lcy::bytra::http::error::success);
                return;
            }
        }
    }
}

template <typename Message>
void basic_parser<Message>::parse_fixed_body(message_type& message,
        lcy::system::error_code& ec)
{
    const std::size_t avail = buf_size_ - buf_pos_;
    std::size_t take = avail;
    if ( take > body_remaining_ ) {
        take = body_remaining_;
    }

    if ( take > 0 ) {
        append_body(message.body_container(), buf_ + buf_pos_, take);
        buf_pos_ += take;
        body_remaining_ -= take;
        body_parsed_ += take;
        parsed_bytes_ += take;
        total_parsed_bytes_ += take;
    }

    if ( body_remaining_ == 0 ) {
        phase_ = phase::done;
        ec = make_http_error_code(lcy::bytra::http::error::success);
    } else {
        ec = make_http_error_code(lcy::bytra::http::error::wait_more);
    }
}

template <typename Message>
const char* basic_parser<Message>::CRLF() const
{
    if ( buf_ == 0 || buf_pos_ + 1 >= buf_size_ ) {
        return 0;
    }
    for ( std::size_t i = buf_pos_; i + 1 < buf_size_; ++i ) {
        if ( buf_[i] == '\r' && buf_[i + 1] == '\n' ) {
            return buf_ + i;
        }
    }
    return 0;
}

template <typename Message>
bool basic_parser<Message>::split_first_line(const char* data,
        std::size_t size, message_type& message) const
{
    const char* const end = data + size;

    if ( message.is_request() ) {
        const char* space1 = std::find(data, end, ' ');
        if ( space1 == end ) {
            return false;
        }
        const char* space2 = std::find(space1 + 1, end, ' ');
        if ( space2 == end ) {
            return false;
        }

        auto m = method_from_string(std::string(data, space1 - data));
        if ( m == method::unknown ) {
            return false;
        }
        message.set_method(m);
        message.set_target(std::string(space1 + 1, space2));

        version v;
        if ( !parse_version(space2 + 1, end, v) ) {
            return false;
        }
        message.set_version(v);
        return true;
    }

    const char* space1 = std::find(data, end, ' ');
    if ( space1 == end ) {
        return false;
    }

    version v;
    if ( !parse_version(data, space1, v) ) {
        return false;
    }
    message.set_version(v);

    const char* space2 = std::find(space1 + 1, end, ' ');
    const char* status_end = ( space2 == end ) ? end : space2;

    int code = 0;
    if ( !parse_uint(space1 + 1, status_end, code) || code < 100 || code > 599 ) {
        return false;
    }

    status st = status_from_string(std::string(space1 + 1, status_end));
    message.set_status(st);

    if ( space2 != end ) {
        message.set_reason(std::string(space2 + 1, end));
    } else {
        message.set_reason(std::string());
    }
    return true;
}

}   // namespace detail
}   // namespace http
}   // namespace bytra
}   // namespace lcy