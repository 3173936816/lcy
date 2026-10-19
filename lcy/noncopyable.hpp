#ifndef __LCY_NONCOPYABLE_HPP__
#define __LCY_NONCOPYABLE_HPP__

namespace lcy
{

class noncopyable
{
protected:
	noncopyable() {}
	~noncopyable() {}

private:
	noncopyable(const noncopyable&);
	noncopyable& operator=(const noncopyable&);
};

}	// namespace lcy

#endif	// __LCY_NONCOPYABLE_HPP__
