#ifndef __LCY_BYTRA_HTTP_RESPONSE_HPP__
#define __LCY_BYTRA_HTTP_RESPONSE_HPP__

#include "lcy/bytra/http/status.hpp"
#include "lcy/bytra/http/detail/message.hpp"

namespace lcy
{
namespace bytra
{
namespace http
{

template <typename BodyContainer>
class response :
    public detail::message<
        detail::response_tag,
        BodyContainer
    >
{
public:
    typedef detail::message<
        detail::response_tag,
        BodyContainer
    > base_type;

    typedef BodyContainer body_container_type;

public:
    response() : status_(status::ok), reason_(status_to_reason(status::ok)) {}

    response(status s, version v) :
        base_type(v), status_(s), reason_(status_to_reason(s)) {}

    response(status s, std::string reason, version v) :
        base_type(v), status_(s), reason_(std::move(reason)) {}

    status get_status() const
    {
        return status_;
    }

    void set_status(status s)
    {
        status_ = s;
        reason_ = status_to_reason(s);
    }

    const std::string& get_reason() const
    {
        return reason_;
    }

    void set_reason(std::string reason)
    {
        reason_ = std::move(reason);
    }

    std::string header_to_string() const
    {
        std::string result;
        result += status_to_string(status_) + " ";
        result += reason_ + " ";
        result += version_to_string(this->get_version()) + "\r\n";
        for ( auto beg = this->field_begin(); beg != this->field_end(); ++beg ) {
            result += beg->first + ": " + beg->second + "\r\n";
        }
        result += "\r\n";

        return result;
    }

private:
    status status_;
    std::string reason_;
};

}   // namespace http
}   // namespace bytra 
}   // namespace lcy

#endif  // __LCY_BYTRA_HTTP_RESPONSE_HPP__