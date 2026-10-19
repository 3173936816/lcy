#ifndef __LCY_ASIO_BUFFER_VIEW_HPP__
#define __LCY_ASIO_BUFFER_VIEW_HPP__

#include <cstdint>

namespace lcy
{
namespace asio
{

class const_buffer_view
{
public:
	const_buffer_view() :
		data_(0), length_(0) {}

	const_buffer_view(const void* data, std::size_t length) :
		data_(data), length_(length) {}

	const_buffer_view operator+(std::size_t offset)
	{
		if ( offset > length_ ) {
			offset = length_;
		}

		return const_buffer_view(
			static_cast<const char*>(data_) + offset,
			length_ - offset);
	}

	const void* data() const
	{
		return data_;
	}

	std::size_t length() const
	{
		return length_;
	}

private:
	const void* data_;
	std::size_t length_;
};

class mutable_buffer_view
{
public:
	mutable_buffer_view() :
		data_(0), length_(0) {}

	mutable_buffer_view(void* data, std::size_t length) :
		data_(data), length_(length) {}

	mutable_buffer_view operator+(std::size_t offset)
	{
		if ( offset > length_ ) {
			offset = length_;
		}

		return mutable_buffer_view(
			static_cast<char*>(data_) + offset,
			length_ - offset);
	}

	void* data()
	{
		return data_;
	}

	std::size_t length() const
	{
		return length_;
	}

private:
	void* data_;
	std::size_t length_;
};

}	// namespace asio
}	// nemspace lcy

#endif	// __LCY_ASIO_BUFFER_VIEW_HPP__
