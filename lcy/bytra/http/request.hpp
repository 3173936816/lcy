#ifndef __LCY_BYTRA_HTTP_DETAIL_REQUEST_HPP__
#define __LCY_BYTRA_HTTP_DETAIL_REQUEST_HPP__

#include "lcy/bytra/http/method.hpp"
#include "lcy/bytra/http/detail/message.hpp"

namespace lcy
{
namespace bytra
{
namespace http
{

template <typename BodyContainer>
class request :
    public detail::message<
        detail::request_tag,
        BodyContainer
    >
{
public:
    typedef detail::message<
        detail::request_tag,
        BodyContainer
    > base_type;

    typedef BodyContainer body_container_type;

public:
    request() : method_(method::get), target_("/") {}

    request(method m, std::string target, version v) :
        base_type(v), method_(m), target_(std::move(target)) {}

    method get_method() const
    {
        return method_;
    }

    void set_method(method m)
    {
        method_ = m;
    }

    const std::string& get_target() const
    {
        return target_;
    }

    void set_target(std::string target)
    {
        target_ = std::move(target);
    }

    std::string header_to_string() const
    {
        std::string result;
        result += method_to_string(method_) + " ";
        result += target_ + " ";
        result += version_to_string(this->get_version()) + "\r\n";
        for ( auto beg = this->field_begin(); beg != this->field_end(); ++beg ) {
            result += beg->first + ": " + beg->second + "\r\n";
        }
        result += "\r\n";

        return result;
    }

private:
    method method_;
    std::string target_;
};

}   // namespace http
}   // namespace bytra
}   // namespace lcy

#endif  // __LCY_BYTRA_HTTP_DETAIL_REQUEST_HPP__