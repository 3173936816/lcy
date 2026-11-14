#ifndef __LCY_LOG4CC_LOGGER_HPP__
#define __LCY_LOG4CC_LOGGER_HPP__

#include <list>
#include <memory>
#include <string>

#include "lcy/noncopyable.hpp"
#include "lcy/log4cc/level.hpp"
#include "lcy/log4cc/record.hpp"
#include "lcy/log4cc/formatter.hpp"
#include "lcy/log4cc/appender_base.hpp"

namespace lcy
{
namespace log4cc
{

class logger :
    private noncopyable
{
public:
    typedef std::list<
        appender_base::sptr_type
    > appender_list_type;

public:
    explicit logger(std::string name, level l = level::debug) :
        name_(std::move(name)), level_(l),
        formatter_(std::make_shared<formatter>()) {}

    const std::string& get_name() const
    {
        return name_;
    }

    void set_level(level l)
    {
        level_ = l;
    }

    level get_level() const
    {
        return level_;
    }

    bool should_log(level l) const
    {
        return l >= level_;
    }

    void set_formatter(formatter::sptr_type f)
    {
		formatter_ = f;
    }

	formatter::sptr_type get_formatter()
	{
		return formatter_;
	}

    void add_appender(appender_base::sptr_type appender)
    {
        if ( appender ) {
            appender->set_formatter(formatter_);
            appenders_.push_back(std::move(appender));
        }
    }

    void log(record::sptr_type rsptr)
    {
        if ( !should_log(rsptr->get_level()) ) {
            return;
        }

        for ( const auto& appender : appenders_ ) {
            appender->log(rsptr);
        }
    }

private:
    std::string name_;
    level level_;
    formatter::sptr_type formatter_;
    appender_list_type appenders_;
};

}   // namespace log4cc
}   // namespace lcy

#endif  // __LCY_LOG4CC_LOGGER_HPP__
