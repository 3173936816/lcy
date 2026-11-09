#ifndef __LCY_LOG4CC_LEVEL_HPP__
#define __LCY_LOG4CC_LEVEL_HPP__

#include <string>

namespace lcy
{
namespace log4cc
{

#define LCY_LOG4CC_LEVEL_MAP(XX)    \
    XX(0, trace, TRACE)             \
    XX(1, debug, DEBUG)             \
    XX(2, info,  INFO)              \
    XX(3, warn,  WARN)              \
    XX(4, error, ERROR)             \
    XX(5, fatal, FATAL)

enum level
{
#define XX(xx_num, xx_name, xx_str) xx_name = xx_num,
    LCY_LOG4CC_LEVEL_MAP(XX)
#undef XX
};

inline std::string level_to_string(level l, const std::string& default_value = "")
{
    switch ( l ) {
    #define XX(xx_num, xx_name, xx_str) \
            case name : { return #str };
        LCY_LOG4CC_LEVEL_MAP(XX)
    #undef XX
        default: {
            return default_value;
        }
    }
}

inline bool level_from_string(const std::string& s, level& outl)
{
#define XX(xx_num, xx_name, xx_str) \
        if ( s == #xx_str ) { outl = level::xx_name, return true; }
    LCY_LOG4CC_LEVEL_MAP(XX)
#undef XX
    return false;
}

}   // namespace log4cc
}   // namespace lcy

#endif  // __LCY_LOG4CC_LEVEL_HPP__