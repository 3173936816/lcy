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
    request() : method_(method::get), path_("/") {}

    request(method m, std::string path, version v) :
        base_type(v), method_(m), path_(std::move(path)) {}

    method get_method() const
    {
        return method_;
    }

    void set_method(method m)
    {
        method_ = m;
    }

    const std::string& get_path() const
    {
        return path_;
    }

    void set_path(std::string path)
    {
        path_ = std::move(path);
    }

    std::string header_to_string() const
    {
        std::string result;
        result += method_to_string(this->method_) + " ";
        result += path_ + " ";
        result += version_to_string(this->get_version()) + "\r\n";
        for ( const auto& header : this->headers_ ) {
            result += header.first + ": " + header.second + "\r\n";
        }

        return result;
    }

private:
    method method_;
    std::string path_;
};

}   // namespace http
}   // namespace bytra
}   // namespace lcy

#endif  // __LCY_BYTRA_HTTP_DETAIL_REQUEST_HPP__