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
    XX(wait_more,            1, Wait more data)                 \
    XX(invalid_state,        2, Invalid state)                  \
    XX(header_too_large,     3, Header too large)               \
    XX(body_too_large,       4, Body too large)                 \
    XX(invalid_header,       5, Invalid header)                 \
    XX(invalid_body,         6, Invalid body)                   \

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