#ifndef __LCY_ASIO_SSL_ERROR_HPP__
#define __LCY_ASIO_SSL_ERROR_HPP__

namespace lcy
{
namespace asio
{
namespace ssl
{

#define LCY_SSL_ERROR_NUMBER_MAP(XX)                               \
    XX(success,                     0,  Success)                   \
    XX(handshake_failed,            1,  Handshake failed)          \
    XX(certificate_verify_failed,   2,  Certificate verify failed) \
    XX(certificate_expired,         3,  Certificate has expired)   \
    XX(self_signed_certificate,     4,  Self-Signed certificate)   \
    XX(unable_to_get_issuer,        5,  Unable to get issuer cert) \
    XX(no_certificate,              6,  No certificate)            \
    XX(no_private_key,              7,  No private key)            \
    XX(bad_certificate,             8,  Bad certificate)           \
    XX(wrong_version,               9,  Wrong version)             \
    XX(unknown_protocol,            10, Unknown protocol)          \
    XX(shutdown_failed,             11, Shutdown failed)           \
    XX(stream_truncated,            12, Stream truncated)          \
    XX(openssl_error,               13, OpenSSL internal error)    \


enum error
{
#define XX(xx_errnum, xx_custom_num, xx_errmsg) \
        xx_errnum = xx_custom_num,
    LCY_SSL_ERROR_NUMBER_MAP(XX)
#undef XX
};

}   // namespace ssl
}   // namespace asio
}   // namespace lcy

#endif  // __LCY_ASIO_SSL_ERROR_HPP__