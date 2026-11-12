#ifndef __LCY_LOG4CC_DETAIL_THROW_LOG4CC_EXCEPTION_HPP__
#define __LCY_LOG4CC_DETAIL_THROW_LOG4CC_EXCEPTION_HPP__

#include <cerrno>
#include <cstring>
#include <exception>
#include <string>

#include "lcy/system/error.hpp"

namespace lcy
{
namespace log4cc
{

class log4cc_exception :
    public std::exception
{
public:
    explicit log4cc_exception(std::string errmsg) :
        errmsg_(std::move(errmsg)) {}

    const char* what() const noexcept override
    {
        return errmsg_.c_str();
    }

private:
    std::string errmsg_;
};

inline void throw_log4cc_exception(lcy::system::error_code ec, std::string usermsg)
{
    std::string msg = "log4cc:{" + usermsg + "}: " + ec.message();
    throw log4cc_exception(std::move(msg));
}

}   // namespace log4cc
}   // namespace lcy

#endif  // __LCY_LOG4CC_DETAIL_THROW_LOG4CC_EXCEPTION_HPP__