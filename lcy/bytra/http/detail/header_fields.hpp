#ifndef __LCY_BYTRA_HTTP_DETAIL_HEADER_FIELDS_HPP__
#define __LCY_BYTRA_HTTP_DETAIL_HEADER_FIELDS_HPP__

#include <list>
#include <string>
#include <unordered_map>

namespace lcy
{
namespace bytra
{
namespace http
{
namespace detail
{

class header_fields
{
public:
    typedef std::pair<
        std::string,
        std::string
    > field_kv_type;

    typedef std::list<
        field_kv_type
    >::const_iterator const_iterator;

public:
    const std::string& get_field(const std::string& name,
        const std::string& default_value = "") const
    {
        auto iter = field_index_.find(name);
        if ( iter != field_index_.end() ) {
            return *(iter->second);
        }

        return default_value;
    }

    void set_field(std::string name, std::string value)
    {
        auto iter = field_list_.insert(field_list_.end(), { name, std::move(value) });
        field_index_.insert({ std::move(name), iter });
    }

    void remove_field(const std::string& name)
    {
        auto iter = field_index_.find(name);
        if ( iter != field_index_.end() ) {
            field_list_.erase(iter->second);
            field_index_.earse(iter);
        }
    }

    

private:
    std::list<field_kv_type> field_list_;

    std::unordered_map<
        std::string,
        std::list<
            field_kv_type
        >::iterator
    > field_index_;
};

}   // namespace detail
}   // namespace http
}   // namespace bytra
}   // namespace lcy

#endif  // __LCY_BYTRA_HTTP_DETAIL_HEADER_FIELDS_HPP__