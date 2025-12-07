#include "asio/src/buffer.h"

namespace lcy {
namespace asio {

ConstBuffer::ConstBuffer(const std::string& strbuf) :
	buf_len_(0),
	buf_ptr_(nullptr)
{
	if ( !strbuf.empty() ) {
		buf_len_ = strbuf.length();
		buf_ptr_ = strbuf.c_str();
	}
}

ConstBuffer::ConstBuffer(const void* buf, size_t length) :
	buf_len_(length),
	buf_ptr_(buf)
{
}

ConstBuffer::~ConstBuffer()
{
}

ConstBuffer ConstBuffer::operator+(size_t offset)
{
	if ( offset > buf_len_ )
		offset = buf_len_;

	return ConstBuffer((const char*)buf_ptr_ + offset,
									buf_len_ - offset);
}

const void* ConstBuffer::data() const
{
	return buf_ptr_;
}

size_t ConstBuffer::length() const
{
	return buf_len_;
}

///////////////////////////////////////////////

MutableBuffer::MutableBuffer(std::string& strbuf) :
	buf_len_(0),
	buf_ptr_(nullptr)
{
	if ( !strbuf.empty() ) {
		buf_len_ = strbuf.length();
		buf_ptr_ = &strbuf[0];
	}
}

MutableBuffer::MutableBuffer(void* buf, size_t length) :
	buf_len_(length),
	buf_ptr_(buf)
{
}

MutableBuffer::~MutableBuffer()
{
}

MutableBuffer::operator ConstBuffer() const
{
	return ConstBuffer(buf_ptr_, buf_len_);
}

MutableBuffer MutableBuffer::operator+(size_t offset)
{
	if ( offset > buf_len_ )
		offset = buf_len_;

	return MutableBuffer((char*)buf_ptr_ + offset,
							  	buf_len_ - offset);
}

void* MutableBuffer::data()
{
	return buf_ptr_;
}

size_t MutableBuffer::length() const
{
	return buf_len_;
}

/////////////////////////////////////////////

ConstBuffer buffer(const std::string& strbuf)
{
	return ConstBuffer(strbuf);
}

ConstBuffer buffer(const void* buf, size_t length)
{
	return ConstBuffer(buf, length);
}

MutableBuffer buffer(std::string& strbuf)
{
	return MutableBuffer(strbuf);
}

MutableBuffer buffer(void* buf, size_t length)
{
	return MutableBuffer(buf, length);
}

}	// namespace asio
}	// namespace lcy
