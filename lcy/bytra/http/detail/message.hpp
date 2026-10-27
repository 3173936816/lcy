#ifndef __LCY_BYTRA_HTTP_MESSAGE_HPP__
#define __LCY_BYTRA_HTTP_MESSAGE_HPP__

#include <string>
#include <unordered_map>

#include "lcy/bytra/http/version.hpp"

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
    typename BodyContainer,
>
class message
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

    void set_version(version version)
    {
        version_ = version;
    }

    const std::string& get_header(const std::string& name,
        const std::string& default_value = "") const
    {
        auto iter = headers_.find(name);
        if ( iter != headers_.end() ) {
            return iter->second;
        }

        return default_value;
    }

    void set_header(std::string name, std::string value)
    {
        headers_[std::move(name)] = std::move(value);
    }

    body_container_type& body_container()
    {
        return body_container_;
    }

protected:
    version version_;
    std::unordered_map<
        std::string,
        std::string
    > headers_;

    body_container_type body_container_;
};

struct request_tag { static constexpr bool value = true; };
struct response_tag{ static constexpr bool value = false; };

}   // namespace detail
}   // namespace http
}   // namespace bytra
}   // namespace lcy

#endif  // __LCY_BYTRA_HTTP_MESSAGE_HPP__