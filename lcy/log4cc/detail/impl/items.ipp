namespace lcy
{
namespace log4cc
{
namespace detail
{

namespace internal_item
{

template <typename RecordSptr>
class level_item :
    public item_base<RecordSptr>
{
public:
    void format(RecordSptr rsptr, std::ostringstream& oss) override
    {
        oss << level_to_string(rsptr->get_level());
    }
};

template <typename RecordSptr>
class pid_item :
    public item_base<RecordSptr>
{
public:
    void format(RecordSptr rsptr, std::ostringstream& oss) override
    {
        oss << rsptr->get_pid();
    }
};

template <typename RecordSptr>
class thread_id_item :
    public item_base<RecordSptr>
{
public:
    void format(RecordSptr rsptr, std::ostringstream& oss) override
    {
        oss << rsptr->get_thread_id();
    }
};

template <typename RecordSptr>
class timestamp_item :
    public item_base<RecordSptr>
{
public:
    timestamp_item(std::string time_format = "%Y-%m-%d %H:%M:%S") :
        time_format_(std::move(time_format)) {}

    void format(RecordSptr rsptr, std::ostringstream& oss) override
    {
        std::time_t t = rsptr->get_timestamp();

        std::tm tm_buf;
        localtime_r(&t, &tm_buf);

        char buf[64];
        std::size_t n = std::strftime(buf,
            sizeof(buf), time_format_.c_str(), &tm_buf);
        if ( n == 0 ) {
            return;
        }

        oss << std::string(buf, n);
    }

private:
    std::string time_format_;
};

template <typename RecordSptr>
class file_name_item :
    public item_base<RecordSptr>
{
public:
    void format(RecordSptr rsptr, std::ostringstream& oss) override
    {
        oss << rsptr->get_file_name();
    }
};

template <typename RecordSptr>
class file_line_item :
    public item_base<RecordSptr>
{
public:
    void format(RecordSptr rsptr, std::ostringstream& oss) override
    {
        oss << rsptr->get_file_line();
    }
};

template <typename RecordSptr>
class logger_name_item :
    public item_base<RecordSptr>
{
public:
    void format(RecordSptr rsptr, std::ostringstream& oss) override
    {
        oss << rsptr->get_logger_name();
    }
};

template <typename RecordSptr>
class message_item :
    public item_base<RecordSptr>
{
public:
    void format(RecordSptr rsptr, std::ostringstream& oss) override
    {
        oss << rsptr->get_message();
    }
};

template <typename RecordSptr>
class literal_item :
    public item_base<RecordSptr>
{
public:
    literal_item(std::string literal) :
        literal_(std::move(literal)) {}

    void format(RecordSptr, std::ostringstream& oss) override
    {
        oss << literal_;
    }

private:
    std::string literal_;
};

template <typename RecordSptr>
class tab_item :
    public item_base<RecordSptr>
{
public:
    void format(RecordSptr, std::ostringstream& oss) override
    {
        oss << '\t';
    }
};

template <typename RecordSptr>
class enter_item :
    public item_base<RecordSptr>
{
public:
    void format(RecordSptr, std::ostringstream& oss) override
    {
        oss << '\n';
    }
};

}   // namespace internal_item

template <typename RecordSptr>
void parse_pattern(const std::string& pattern,
    std::list<typename item_base<RecordSptr>::sptr_type>& items)
{
    std::string literal;

    auto flush_literal = [&]() {
        if ( !literal.empty() ) {
            items.push_back(std::make_shared<
                internal_item::literal_item<
                    RecordSptr
                >
            >(std::move(literal)));
            literal.clear();
        }
    };

    for ( std::size_t i = 0; i < pattern.size(); ++i ) {
        char c = pattern[i];

        if ( c != '%' ) {
            literal.push_back(c);
            continue;
        }

        if ( i + 1 >= pattern.size() ) {
            literal.push_back('%');
            break;
        }

        char spec = pattern[++i];

        switch ( spec ) {
            case '%': {
                literal.push_back('%');
                break;
            }
            case 'l': {
                flush_literal();
                items.push_back(std::make_shared<
                    internal_item::level_item<
                        RecordSptr
                    >
                >());
                break;
            }
            case 'p': {
                flush_literal();
                items.push_back(std::make_shared<
                    internal_item::pid_item<
                        RecordSptr
                    >
                >());
                break;
            }
            case 't': {
                flush_literal();
                items.push_back(std::make_shared<
                    internal_item::thread_id_item<
                        RecordSptr
                    >
                >());
                break;
            }
            case 'd': {
                flush_literal();

                std::string fmt = "%Y-%m-%d %H:%M:%S";

                if ( i + 1 < pattern.size() && pattern[i + 1] == '{' ) {
                    std::size_t end = pattern.find('}', i + 2);

                    if ( end != std::string::npos ) {
                        fmt = pattern.substr(i + 2, end - i - 2);
                        i = end;
                    }
                }

                items.push_back(std::make_shared<
                    internal_item::timestamp_item<
                        RecordSptr
                    >
                >(std::move(fmt)));
                break;
            }
            case 'f': {
                flush_literal();
                items.push_back(std::make_shared<
                    internal_item::file_name_item<
                        RecordSptr
                    >
                >());
                break;
            }
            case 'n': {
                flush_literal();
                items.push_back(std::make_shared<
                    internal_item::file_line_item<
                        RecordSptr
                    >
                >());
                break;
            }
            case 'c': {
                flush_literal();
                items.push_back(std::make_shared<
                    internal_item::logger_name_item<
                        RecordSptr
                    >
                >());
                break;
            }
            case 'm': {
                flush_literal();
                items.push_back(std::make_shared<
                    internal_item::message_item<
                        RecordSptr
                    >
                >());
                break;
            }
            case 'T': {
                flush_literal();
                items.push_back(std::make_shared<
                    internal_item::tab_item<
                        RecordSptr
                    >
                >());
                break;
            }
            case 'E': {
                flush_literal();
                items.push_back(std::make_shared<
                    internal_item::enter_item<
                        RecordSptr
                    >
                >());
                break;
            }
            default: {
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