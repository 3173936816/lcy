namespace lcy
{
namespace log4cc
{
namespace detail
{

namespace items
{

class literal :
    public formatter::item
{
public:
    explicit literal(std::string s) :
        s_(std::move(s)) {}

    void get_item(event&, std::ostringstream& oss) const override
    {
        oss << s_;
    }

private:
    std::string s_;
};

class level :
    public formatter::item
{
public:
    void get_item(event& e, std::ostringstream& oss) const override
    {
        oss << level_to_string(e.level_, "<error_fromat>");
    }
};

class pid :
    public formatter::item
{
public:
    void get_item(event& e, std::ostringstream& oss) const override
    {
        oss << e.pid_;
    }
};

class thread_id :
    public formatter::item
{
public:
    void get_item(event& e, std::ostringstream& oss) const override
    {
        oss << e.thread_id_;
    }
};

class time :
    public formatter::item
{
public:
    void get_item(event& e, std::ostringstream& oss) const override
    {
        char buf[32];
        tm tm_buf;
        ::localtime_r(&e.timestamp_, &tm_buf);
        ::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_buf);
        oss << buf;
    }
};

class file :
    public formatter::item
{
public:
    void get_item(event& e, std::ostringstream& oss) const override
    {
        oss << e.file_name_;
    }
};

class line :
    public formatter::item
{
public:
    void get_item(event& e, std::ostringstream& oss) const override
    {
        oss << e.file_line_;
    }
};

class logger_name :
    public formatter::item
{
public:
    void get_item(event& e, std::ostringstream& oss) const override
    {
        oss << e.logger_name_;
    }
};

class message :
    public formatter::item
{
public:
    void get_item(event& e, std::ostringstream& oss) const override
    {
        oss << e.oss_.str();
    }
};

}   // namespace item

formatter::formatter() :
    pattern_("[%d] [%l] [pid:%p] [tid:%t] [%f:%n] [%c] %m")
{
    parse_pattern();
}

formatter::formatter(std::string pattern) :
    pattern_(std::move(pattern))
{
    parse_pattern();
}

formatter::~formatter()
{
    clear_items();
}

const std::string& formatter::get_pattern() const
{
    return pattern_;
}

void formatter::set_pattern(std::string pattern)
{
    pattern_ = std::move(pattern);
    clear_items();
    parse_pattern();
}

std::string formatter::format(event& e) const
{
    std::ostringstream oss;

    for ( auto i : items_ ) {
        i->get_item(e, oss);
    }

    return oss.str();
}

void formatter::clear_items()
{
    for ( auto i : items_ ) {
        delete i;
    }

    items_.clear();
}

void formatter::parse_pattern()
{
    std::string literal;

    auto flush_literal = [&]() {
        if ( !literal.empty() ) {
            items_.push_back(new items::literal(std::move(literal)));
            literal.clear();
        }
    };

    for ( std::size_t i = 0; i < pattern_.size(); ++i ) {
        char c = pattern_[i];

        if ( c != '%' ) {
            literal.push_back(c);
            continue;
        }

        if ( i + 1 >= pattern_.size() ) {
            literal.push_back('%');
            break;
        }

        char spec = pattern_[++i];

        switch ( spec ) {
            case '%': {
                literal.push_back('%');
                break;
            }
            case 'l': {
                flush_literal();
                items_.push_back(new items::level());
                break;
            }
            case 'p': {
                flush_literal();
                items_.push_back(new items::pid());
                break;
            }
            case 't': {
                flush_literal();
                items_.push_back(new items::thread_id());
                break;
            }
            case 'd': {
                flush_literal();
                items_.push_back(new items::time());
                break;
            }
            case 'f': {
                flush_literal();
                items_.push_back(new items::file());
                break;
            }
            case 'n': {
                flush_literal();
                items_.push_back(new items::line());
                break;
            }
            case 'c': {
                flush_literal();
                items_.push_back(new items::logger_name());
                break;
            }
            case 'm': {
                flush_literal();
                items_.push_back(new items::message());
                break;
            }
            default : {
                literal.push_back('%');
                literal.push_back(spec);
                break;
            }
        }
    }

    flush_literal();
}

}   // namespace detail
}   // namespace log4cc
}   // namespace lcy