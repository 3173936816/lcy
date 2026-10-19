#ifndef __LCY_OBJECT_POOL_HPP__
#define __LCY_OBJECT_POOL_HPP__

#include <list>
#include <memory>
#include <unordered_map>

#include "lcy/noncopyable.hpp"

namespace lcy
{

template <typename Object>
class object_pool :
	public noncopyable
{
public:
	object_pool() {}

	~object_pool()
	{
		for ( auto ob : live_list_ ) {
			delete ob;
		}
		for ( auto ob : free_list_ ) {
			delete ob;
		}
	}

	template <typename ...Args>
	Object* alloc(Args&&... args)
	{
		Object* ob = 0;

		if ( free_list_.empty() ) {
			ob = new Object(std::forward<Args>(args)...);
		} else {
			ob = free_list_.front();
			free_list_.pop_front();
		}
		live_list_.push_front(ob);
		live_list_index_.insert({ob, live_list_.begin()});

		return ob;
	}

	void free(Object* ob)
	{
		auto iter = live_list_index_.find(ob);
		if ( iter != live_list_index_.end() ) {
			live_list_.erase(iter->second);
			live_list_index_.erase(iter);

			free_list_.push_front(ob);
		}
	}

private:
	std::list<Object*> live_list_;
	std::list<Object*> free_list_;
	std::unordered_map<
		Object*,
		typename std::list<
			Object*
		>::iterator
	> live_list_index_; 
};

}	// namespace lcy

#endif	// __LCY_OBJECT_POOL_HPP__
