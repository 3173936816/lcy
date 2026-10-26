#ifndef __LCY_BYTRA_HTTP_MESSAGE_HPP__
#define __LCY_BYTRA_HTTP_MESSAGE_HPP__

#include <string>
#include <unordered_map>

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
    bool is_request() const
    {
        return IsRequest::value;
    }

    void set_header(std::string name, std::string value)
    {
        headers_[std::move(name)] = std::move(value);
    }

    const std::string& get_header(const std::string& name,
        const std::string& default_value = std::string()) const
    {
        auto it = headers_.find(name);
        if ( it != headers_.end() ) {
            return it->second;
        }
        return default_value;
    }

    body_container_type& body_container()
    {
        return body_container_;
    }

private:
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