#ifndef __LCY_ASIO_DYNAMIC_BUFFER_H__
#define __LCY_ASIO_DYNAMIC_BUFFER_H__

#include <vector>
#include <memory>
#include <string>

namespace lcy {
namespace asio {

class DynamicBuffer {
public:
	DynamicBuffer(size_t expend_size = 1024);
	~DynamicBuffer();

	char* writeBegin();
	const char* readBegin() const;
	void read(size_t size);
	void write(size_t size);
	size_t dataBytes() const;
	size_t availableBytes() const;
	size_t uselessBytes() const;
	void reserve(size_t size);
	std::string readAllToString();

	void write8Bits(int8_t value);
	void write8Bits(uint8_t value);
	void write16Bits(int16_t value);
	void write16Bits(uint16_t value);
	void write32Bits(int32_t value);
	void write32Bits(uint32_t value);
	void write64Bits(int64_t value);
	void write64Bits(uint64_t value);

	bool read8Bits(int8_t* value);
	bool read8Bits(uint8_t* value);
	bool read16Bits(int16_t* value);
	bool read16Bits(uint16_t* value);
	bool read32Bits(int32_t* value);
	bool read32Bits(uint32_t* value);
	bool read64Bits(int64_t* value);
	bool read64Bits(uint64_t* value);

	bool peek8Bits(int8_t* value, size_t offset = 0);
	bool peek8Bits(uint8_t* value, size_t offset = 0);
	bool peek16Bits(int16_t* value, size_t offset = 0);
	bool peek16Bits(uint16_t* value, size_t offset = 0);
	bool peek32Bits(int32_t* value, size_t offset = 0);
	bool peek32Bits(uint32_t* value, size_t offset = 0);
	bool peek64Bits(int64_t* value, size_t offset = 0);
	bool peek64Bits(uint64_t* value, size_t offset = 0);

private:
	typedef std::vector<char> CharVector;

	size_t read_index_;
	size_t write_index_;
	size_t expand_size_;
	CharVector char_vector_;
};

}	// namespace asio
}	// namespace lcy

#endif // __LCY_ASIO_DYNAMIC_BUFFER_H__
