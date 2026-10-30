#ifndef __LCY_BYTRA_HTTP_MESSAGE_HPP__
#define __LCY_BYTRA_HTTP_MESSAGE_HPP__

#include <list>
#include <string>
#include <unordered_map>

#include "lcy/bytra/http/version.hpp"
#include "lcy/bytra/http/detail/header_fields.hpp"

namespace lcy
{
namespace bytra
{
namespace http
{
namespace detail
{

template <
    typename IsRequest,
    typename BodyContainer
>
class message :
    public header_fields
{
public:
    typedef BodyContainer body_container_type;

public:
    explicit message(version v = version::http_1_1) :
        version_(v) {}

    bool is_request() const
    {
        return IsRequest::value;
    }

    version get_version() const
    {
        return version_;
    }

    void set_version(version v)
    {
        version_ = v;
    }

    body_container_type& body_container()
    {
        return body_container_;
    }

private:
    version version_;
    body_container_type body_container_;
};

struct request_tag { static constexpr bool value = true; };
struct response_tag{ static constexpr bool value = false; };

}   // namespace detail
}   // namespace http
}   // namespace bytra
}   // namespace lcy

#endif  // __LCY_BYTRA_HTTP_MESSAGE_HPP__