#ifndef __LCY_LOG4CC_DETAIL_FILE_OPS_HPP__
#define __LCY_LOG4CC_DETAIL_FILE_OPS_HPP__

#include <cstddef>
#include <string>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include "lcy/log4cc/detail/throw_log4cc_exception.hpp"

namespace lcy
{
namespace log4cc
{
namespace detail
{

namespace file_ops
{

inline std::string file_stem(const std::string& full_path)
{
    std::string base = filename(full_path);
    std::size_t dot = base.find_last_of('.');

    if ( dot == std::string::npos || dot == 0 ) {
        return base;
    }

    return base.substr(0, dot);
}

inline std::string filename(const std::string& full_path)
{
    std::size_t slash = full_path.find_last_of('/');

    if ( slash == std::string::npos ) {
        return full_path;
    }

    return full_path.substr(slash + 1);
}

inline std::string ext(const std::string& full_path)
{
    std::string name = filename(full_path);
    std::size_t dot = name.find_last_of('.');

    if ( dot == std::string::npos || dot == 0 ) {
        return "";
    }

    return name.substr(dot);
}

inline std::size_t scan_file_index(const std::string& full_path)
{
    std::string dir = ".";
    std::string base = filename(full_path);

    std::size_t slash = full_path.find_last_of('/');
    if ( slash != std::string::npos ) {
        dir = full_path.substr(0, slash);
    }

    DIR* d = ::opendir(dir.c_str());
    if ( !d ) {
        return 0;
    }

    std::size_t max_index = 0;
    std::string prefix = base + ".";

    struct dirent* entry = 0;
    while ( (entry = ::readdir(d)) != 0 ) {
        std::string name = entry->d_name;

        if ( name.size() <= prefix.size() ) {
            continue;
        }

        if ( name.compare(0, prefix.size(), prefix) != 0 ) {
            continue;
        }

        std::string rest = name.substr(prefix.size());

        if ( rest.empty() ) {
            continue;
        }

        bool all_digit = true;
        for ( char c : rest ) {
            if ( c < '0' || c > '9' ) {
                all_digit = false;
                break;
            }
        }

        if ( !all_digit ) {
            continue;
        }

        std::size_t index = static_cast<std::size_t>(std::stoul(rest));
        if ( index > max_index ) {
            max_index = index;
        }
    }

    ::closedir(d);

    return max_index;
}

inline void renamefile(const std::string& from, const std::string& to)
{
    if ( ::rename(from.c_str(), to.c_str()) != 0 ) {
        throw_log4cc_exception("renamefile rename "
            + from + " -> " + to + " error");
    }
}

inline std::size_t file_size(const std::string& full_path)
{
    struct stat st;

    if ( ::stat(full_path.c_str(), &st) != 0 ) {
        return 0;
    }

    return static_cast<std::size_t>(st.st_size);
}

}   // namespace file_ops

}   // namespace detail
}   // namespace log4cc
}   // namespace lcy

#endif  // __LCY_LOG4CC_DETAIL_FILE_OPS_HPP__