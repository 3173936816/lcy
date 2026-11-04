#ifndef __LCY_ASIO_SSL_VERIFY_MODE_HPP__
#define __LCY_ASIO_SSL_VERIFY_MODE_HPP__

#include <openssl/ssl.h>

namespace lcy
{
namespace asio
{
namespace ssl
{

enum verify_mode
{
    verify_none = SSL_VERIFY_NONE,
    verify_peer = SSL_VERIFY_PEER,
    verify_fail_if_no_peer_cert = SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
    verify_client_once = SSL_VERIFY_CLIENT_ONCE
};

}   // namespace ssl
}   // namespace asio
}   // namespace lcy

#endif  // __LCY_ASIO_SSL_VERIFY_MODE_HPP__