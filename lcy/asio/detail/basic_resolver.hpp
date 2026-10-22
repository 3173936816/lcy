#ifndef __LCY_ASIO_DETAIL_BASIC_RESOLVER_HPP__
#define __LCY_ASIO_DETAIL_BASIC_RESOLVER_HPP__

#include <string>
#include <functional>

#include "lcy/noncopyable.hpp"
#include "lcy/asio/detail/socket_ops.hpp"
#include "lcy/asio/detail/throw_asio_exception.hpp"

namespace lcy
{
namespace asio
{
namespace detail
{

template <
    typename Protocol,
    typename Endpoint
>
class basic_resolver :
    private noncopyable
{
public:
    typedef Protocol protocol_type;
    typedef Endpoint endpoint_type;

    typedef std::function<
        void (
            lcy::system::error_code ec
        )
    > resolve_func_type;

public:
    template <typename Container>
    void resolve(std::string host,
        std::string service, Protocol proto,
        Container& endpoints, lcy::system::error_code& ec)
    {
        socket_ops::getaddrinfo<
            protocol_type,
            endpoint_type
        >(std::move(host), std::move(service), proto, endpoints, ec);
    }

    template <typename Container>
    void resolve(std::string host,
        std::string service, Protocol proto, Container& endpoints)
    {
        lcy::system::error_code ec;
        socket_ops::getaddrinfo<
            protocol_type,
            endpoint_type
        >(std::move(host), std::move(service), proto, endpoints, ec);
        if ( ec ) {
            throw_asio_exception(ec, "basic_resolver::resolve");
        }
    }
};

}   // namespace detail
}   // namespace asio
}   // namespace lcy

#endif  // __LCY_ASIO_DETAIL_BASIC_RESOLVER_HPP__