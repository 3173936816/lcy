#ifndef __LCY_BYTRA_HTTP_DETAIL_REQUEST_HPP__
#define __LCY_BYTRA_HTTP_DETAIL_REQUEST_HPP__

#include "lcy/bytra/http/detail/message.hpp"

namespace lcy
{
namespace bytra
{
namespace http
{
namespace detail
{

template <typename BodyContainer>
class request :
    public message<
        request_tag,
        BodyContainer
    >
{
public:
    typedef BodyContainer body_container_type;

public:
    request() : method_(0) {}

private:
    int method_;
};

}   // namespace detail
}   // namespace http
}   // namespace bytra
}   // namespace lcy

#endif  // __LCY_BYTRA_HTTP_DETAIL_REQUEST_HPP__