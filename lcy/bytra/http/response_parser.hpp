#ifndef __LCY_BYTRA_HTTP_RESPONSE_PARSER_HPP__
#define __LCY_BYTRA_HTTP_RESPONSE_PARSER_HPP__

#include "lcy/bytra/http/response.hpp"
#include "lcy/bytra/http/detail/basic_parser.hpp"

namespace lcy
{
namespace bytra
{
namespace http
{

typedef detail::basic_parser<
    response
> response_parser;

}   // namespace http
}   // namespace bytra
}   // namespace lcy

#endif  // __LCY_BYTRA_HTTP_RESPONSE_PARSER_HPP__