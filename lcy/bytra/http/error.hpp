#ifndef __LCY_BYTRA_HTTP_ERROR_HPP__
#define __LCY_BYTRA_HTTP_ERROR_HPP__

namespace lcy
{
namespace bytra
{
namespace http
{

#define LCY_BYTRA_HTTP_ERROR_NUMBER_MAP(XX)                     \
    XX(success,              0, Success)                        \
    XX(bad_protocol,         1, Bad protocol)                   \
    XX(phase_mismatch,       2, Phase mismatch)                 \
    XX(first_line_limit,     3, First line limit exceeded)      \
    XX(field_line_limit,     4, Field line limit exceeded)      \
    XX(field_total_limit,    5, Field total limit exceeded)     \
    XX(chunk_payload_limit,  6, Chunk payload limit exceeded)   \
    XX(body_total_limit,     7, Body total limit exceeded)      \


enum error
{
#define XX(xx_errnum, xx_custom_num, xx_errmsg) \
        xx_errnum = xx_custom_num,
    LCY_BYTRA_HTTP_ERROR_NUMBER_MAP(XX)
#undef XX
};

}   // namespace http
}   // namespace bytra
}   // namespace lcy

#endif  // __LCY_BYTRA_HTTP_ERROR_HPP__