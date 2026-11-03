#ifndef __LCY_SSL_VERIFY_MODE_HPP__
#define __LCY_SSL_VERIFY_MODE_HPP__

namespace lcy
{
namespace ssl
{

enum verify_mode
{
    verify_none = 0x0000,
    verify_peer = 0x0001,
    verify_fail_if_no_peer_cert = 0x0010,
    verify_client_once = 0x0100
};

}   // namespace ssl
}   // namespace lcy

#endif  // __LCY_SSL_VERIFY_MODE_HPP__