#ifndef __LCY_BYTRA_HTTP_VERSION_HPP__
#define __LCY_BYTRA_HTTP_VERSION_HPP__

#include <string>

namespace lcy
{
namespace bytra
{
namespace http
{

enum version
{
    http_1_0 = 0,
    http_1_1 = 1,
    http_unknown = 2,
};

inline std::string version_to_string(version v,
    std::string default_value = "")
{
    if ( v == version::http_1_0 ) {
        return "HTTP/1.0";
    } else if ( v == version::http_1_1 ) {
        return "HTTP/1.1";
    }

    return default_value;
}

inline version version_from_string(const std::string& s)
{
    if ( s == "HTTP/1.0" ) {
        return version::http_1_0;
    } else if ( s == "HTTP/1.1" ) {
        return version::http_1_1;
    }

    return version::http_unknown;
}

}   // namespace http
}   // namespace bytra
}   // namespace lcy

#endif  // __LCY_BYTRA_HTTP_VERSION_HPP__