#ifndef __LCY_BYTRA_HTTP_DETAIL_HEADER_FIELDS_HPP__
#define __LCY_BYTRA_HTTP_DETAIL_HEADER_FIELDS_HPP__

#include <list>
#include <string>
#include <iterator>
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
	typedef std::pair<std::string, std::string> field_type;
	typedef std::list<field_type>::const_iterator const_iterator;

public:
	const_iterator set_field(std::string key, std::string value)
	{
        auto key_scope_iter = key_scope_index_.find(key);
        if ( key_scope_iter != key_scope_index_.end() ) {
            auto key_scope_last = key_scope_iter->second.second;
            auto key_scope_new_last = fields_.insert(++key_scope_last, { std::move(key), std::move(value) });
            key_scope_iter->second.second = key_scope_new_last;

            return key_scope_new_last;
        }

        auto iter = fields_.insert(fields_.end(), { key, std::move(value) });
        key_scope_index_[std::move(key)] = { iter, iter };

        return iter;
	}

	const_iterator get_field(const std::string& key) const
	{
		auto key_scope_iter = key_scope_index_.find(key);
		if ( key_scope_iter != key_scope_index_.end() ) {
			return key_scope_iter->second.first;
		}

		return fields_.end();
	}

	void remove_field(const_iterator iter)
	{
		auto key_scope_iter = key_scope_index_.find(iter->first);
		if ( key_scope_iter != key_scope_index_.end() ) {
            if ( key_scope_iter->second.first == key_scope_iter->second.second ) {
                fields_.erase(iter);
                key_scope_index_.erase(key_scope_iter);
                return;
            }

			if ( iter == key_scope_iter->second.first ) {
				++key_scope_iter->second.first;
			} else if ( iter == key_scope_iter->second.second ) {
                --key_scope_iter->second.second;
            }

			fields_.erase(iter);
		}
	}

	void remove_fields(const std::string& key)
	{
		auto key_scope_iter = key_scope_index_.find(key);
		if ( key_scope_iter != key_scope_index_.end() ) {
			fields_.erase(key_scope_iter->second.first, ++key_scope_iter->second.second);
			key_scope_index_.erase(key_scope_iter);
		}
	}

	std::size_t count(const std::string& key) const
	{
		std::size_t result = 0;

		auto key_scope_iter = key_scope_index_.find(key);
		if ( key_scope_iter != key_scope_index_.end() ) {
			result = static_cast<std::size_t>(std::distance(
				key_scope_iter->second.first, key_scope_iter->second.second));
            ++result;
		}

		return result;
	}

    void remove_all_fields()
    {
        fields_.clear();
        key_scope_index_.clear();
    }

	const_iterator field_begin() const
	{
		return fields_.begin();
	}

	const_iterator field_end() const
	{
		return fields_.end();
	}

private:
	typedef std::pair<
		std::list<field_type>::iterator,
		std::list<field_type>::iterator
	> key_scope_type;

	std::list<field_type> fields_;
	std::unordered_map<
		std::string,
		key_scope_type
	> key_scope_index_;
};

}   // namespace detail
}   // namespace http
}   // namespace bytra
}   // namespace lcy

#endif  // __LCY_BYTRA_HTTP_DETAIL_HEADER_FIELDS_HPP__
