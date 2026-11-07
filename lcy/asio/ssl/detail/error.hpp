#ifndef __LCY_ASIO_SSL_DETAIL_ERROR_HPP__
#define __LCY_ASIO_SSL_DETAIL_ERROR_HPP__

#include <openssl/err.h>

#include "lcy/system/error.hpp"
#include "lcy/asio/ssl/error.hpp"

namespace lcy
{
namespace asio
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
        return "asio.ssl";
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

inline lcy::system::error_code map_x509_verify_error(int x509_err)
{
    switch ( x509_err ) {
        case X509_V_ERR_CERT_HAS_EXPIRED:
        case X509_V_ERR_CRL_HAS_EXPIRED:
            return make_ssl_error_code(error::certificate_expired);

        case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
        case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
            return make_ssl_error_code(error::self_signed_certificate);

        case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
        case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
        case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
            return make_ssl_error_code(error::unable_to_get_issuer);

        case X509_V_ERR_CERT_REJECTED:
        case X509_V_ERR_CERT_UNTRUSTED:
        case X509_V_ERR_CERT_SIGNATURE_FAILURE:
        case X509_V_ERR_CRL_SIGNATURE_FAILURE:
            return make_ssl_error_code(error::bad_certificate);

        default:
            return make_ssl_error_code(error::certificate_verify_failed);
    }
}

inline lcy::system::error_code map_ssl_error(SSL* ssl, int ret)
{
    int err = ::SSL_get_error(ssl, ret);

    switch ( err ) {
        case SSL_ERROR_NONE:
            return make_ssl_error_code(error::success);

        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
            return make_ssl_error_code(error::success);

        case SSL_ERROR_ZERO_RETURN:
            return make_ssl_error_code(error::stream_truncated);

        case SSL_ERROR_SYSCALL:
            if ( errno != 0 ) {
                return lcy::system::error_code(errno);
            }
            return make_ssl_error_code(error::stream_truncated);

        case SSL_ERROR_SSL: {
            unsigned long e = ::ERR_peek_last_error();
            int reason = ERR_GET_REASON(e);
            int lib = ERR_GET_LIB(e);

            if ( lib == ERR_LIB_SSL ) {
                switch ( reason ) {
                    case SSL_R_CERTIFICATE_VERIFY_FAILED:
                        return map_x509_verify_error(
                            ::SSL_get_verify_result(ssl));

                    case SSL_R_UNKNOWN_PROTOCOL:
                    case SSL_R_UNKNOWN_SSL_VERSION:
                    case SSL_R_WRONG_VERSION_NUMBER:
                        return make_ssl_error_code(error::unknown_protocol);

                    case SSL_R_UNSUPPORTED_PROTOCOL:
                    case SSL_R_VERSION_TOO_LOW:
                    case SSL_R_VERSION_TOO_HIGH:
                        return make_ssl_error_code(error::wrong_version);

                    case SSL_R_NO_SHARED_CIPHER:
                        return make_ssl_error_code(error::unknown_protocol);

                    case SSL_R_UNEXPECTED_EOF_WHILE_READING:
                        return make_ssl_error_code(error::stream_truncated);

                    default:
                        return make_ssl_error_code(error::handshake_failed);
                }
            }

            if ( lib == ERR_LIB_X509 ) {
                return map_x509_verify_error(::SSL_get_verify_result(ssl));
            }

            return make_ssl_error_code(error::handshake_failed);
        }

        default:
            return make_ssl_error_code(error::handshake_failed);
    }
}

inline unsigned long last_openssl_error()
{
    return ::ERR_peek_last_error();
}

}   // namespace detail
}   // namespace ssl
}   // namespace asio
}   // namespace lcy

#endif  // __LCY_ASIO_SSL_DETAIL_ERROR_HPP__