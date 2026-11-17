#ifndef __LCY_LOG4CC_LOGGER_HPP__
#define __LCY_LOG4CC_LOGGER_HPP__

#include <list>
#include <memory>
#include <string>

#include "lcy/noncopyable.hpp"
#include "lcy/log4cc/level.hpp"
#include "lcy/log4cc/record.hpp"
#include "lcy/log4cc/formatter.hpp"
#include "lcy/log4cc/sink_base.hpp"

namespace lcy
{
namespace log4cc
{

class logger :
    private noncopyable
{
public:
    typedef std::list<
        sink_base::sptr_type
    > sink_list_type;

    typedef std::shared_ptr<logger> sptr_type;

public:
    explicit logger(std::string name, level l = level::debug) :
        name_(std::move(name)), level_(l),
        formatter_(std::make_shared<formatter>()) {}

    const std::string& get_name() const
    {
        return name_;
    }

    level get_level() const
    {
        return level_;
    }

    void set_level(level l)
    {
        level_ = l;
    }

    bool should_log(level l) const
    {
        return l >= level_;
    }

    formatter::sptr_type get_formatter()
	{
		return formatter_;
	}

    void set_formatter(formatter::sptr_type f)
    {
		formatter_ = f;
    }

    void add_sink(sink_base::sptr_type sink)
    {
        if ( sink ) {
            if ( !sink->get_formatter() ) {
                sink->set_formatter(formatter_);
            }
            sinks_.push_back(std::move(sink));
        }
    }

    void reload()
    {
        for ( const auto& sink : sinks_ ) {
            sink->reload();
        }
    }

    void flush()
    {
        for ( const auto& sink : sinks_ ) {
            sink->flush();
        }
    }

    void log(record::sptr_type rsptr)
    {
        if ( !rsptr || !should_log(rsptr->get_level()) ) {
            return;
        }

        for ( const auto& sink : sinks_ ) {
            sink->log(rsptr);
        }
    }

private:
    std::string name_;
    level level_;
    formatter::sptr_type formatter_;
    sink_list_type sinks_;
};

}   // namespace log4cc
}   // namespace lcy

#endif  // __LCY_LOG4CC_LOGGER_HPP__
