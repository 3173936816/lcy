#ifndef __LCY_ASIO_BUFFER_H__
#define __LCY_ASIO_BUFFER_H__

#include <string>
#include <vector>

namespace lcy {
namespace asio {

class ConstBuffer {
public:
	ConstBuffer(const std::string& strbuf);
	ConstBuffer(const void* buf, size_t length);
	~ConstBuffer();

	ConstBuffer operator+(size_t offset);

	const void* data() const;
	size_t length() const;

private:
	size_t buf_len_;
	const void* buf_ptr_;
};


class MutableBuffer {
public:
	MutableBuffer(std::string& strbuf);
	MutableBuffer(void* buf, size_t length);
	~MutableBuffer();

	operator ConstBuffer() const;
	MutableBuffer operator+(size_t offset);

	void* data();
	size_t length() const;

private:
	size_t buf_len_;
	void* buf_ptr_;
};

ConstBuffer buffer(const std::string& strbuf);
ConstBuffer buffer(const void* buf, size_t length);
MutableBuffer buffer(std::string& strbuf);
MutableBuffer buffer(void* buf, size_t length);

}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_BUFFER_H__
