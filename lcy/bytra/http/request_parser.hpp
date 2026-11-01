#ifndef __LCY_BYTRA_HTTP_REQUEST_PARSER_HPP__
#define __LCY_BYTRA_HTTP_REQUEST_PARSER_HPP__

#include "lcy/bytra/http/request.hpp"
#include "lcy/bytra/http/detail/basic_parser.hpp"

namespace lcy
{
namespace bytra
{
namespace http
{

typedef detail::basic_parser<
    request
> request_parser;

}   // namespace http
}   // namespace bytra
}   // namespace lcy

#endif  // __LCY_BYTRA_HTTP_REQUEST_PARSER_HPP__