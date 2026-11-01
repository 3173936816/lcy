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

inline bool version_from_string(const std::string& s, version& v)
{
    if ( s == "HTTP/1.0" ) {
        v = version::http_1_0;
        return true;
    } else if ( s == "HTTP/1.1" ) {
        v = version::http_1_1;
        return true;
    }

    return false;
}

}   // namespace http
}   // namespace bytra
}   // namespace lcy

#endif  // __LCY_BYTRA_HTTP_VERSION_HPP__