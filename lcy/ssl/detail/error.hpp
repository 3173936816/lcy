#ifndef __LCY_SSL_DETAIL_ERROR_HPP__
#define __LCY_SSL_DETAIL_ERROR_HPP__

#include <openssl/err.h>

#include "lcy/system/error.hpp"

namespace lcy
{
namespace ssl
{
namespace detail
{

class ssl_error_category :
    public lcy::system::error_category
{
private:
    friend const lcy::system::error_category& fetch_ssl_category();

    ssl_error_category() {}

public:
    const char* name() const override
    {
        return "ssl";
    }

    const char* message(int errnum) const override
    {
        const char* msg = ::ERR_reason_error_string(
            static_cast<unsigned long>(errnum));

        return msg ? msg : "unknown ssl error";
    }
};

inline const lcy::system::error_category& fetch_ssl_category()
{
    static ssl_error_category m;
    return m;
}

inline lcy::system::error_code make_ssl_error_code()
{
    unsigned long prev_error = 0;
    unsigned long last_error = 0;

    do {
        last_error = prev_error;
        prev_error = ::ERR_get_error();
    } while ( prev_error );

    if ( last_error == 0 ) {
        return lcy::system::error_code();
    }

    return lcy::system::error_code(
        ERR_GET_REASON(last_error), fetch_ssl_category());
}

}   // namespace detail
}   // namespace ssl
}   // namespace lcy

#endif  // __LCY_SSL_DETAIL_ERROR_HPP__