#ifndef __LCY_SSL_DETAIL_ERROR_HPP__
#define __LCY_SSL_DETAIL_ERROR_HPP__

#include <openssl/err.h>

#include "lcy/system/error.hpp"
#include "lcy/ssl/error.hpp"

namespace lcy
{
namespace ssl
{
namespace detail
{

class ssl_category :
    public lcy::system::error_category
{
private:
    friend const lcy::system::error_category& fetch_ssl_category();

    ssl_category() {}

public:
    const char* name() const override
    {
        return "ssl";
    }

    const char* message(int errnum) const override
    {
        switch ( errnum ) {
        #define XX(xx_errnum, xx_custom_num, xx_errmsg) \
                case xx_errnum : { return #xx_errmsg; break; }
            LCY_SSL_ERROR_NUMBER_MAP(XX)
        #undef XX
            default : {
                return "Unknown error";
            }
        }
    }
};

inline const lcy::system::error_category& fetch_ssl_category()
{
    static ssl_category m;
    return m;
}

inline lcy::system::error_code make_ssl_error_code(int errnum = error::success)
{
    return lcy::system::error_code(errnum, fetch_ssl_category());
}

}   // namespace detail
}   // namespace ssl
}   // namespace lcy

#endif  // __LCY_SSL_DETAIL_ERROR_HPP__