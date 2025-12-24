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

	void writeInt8(int8_t value);
	void writeUint8(uint8_t value);
	void writeInt16(int16_t value);
	void writeUint16(uint16_t value);
	void writeInt32(int32_t value);
	void writeUint32(uint32_t value);
	void writeInt64(int64_t value);
	void writeUint64(uint64_t value);

	bool readInt8(int8_t* value);
	bool readUint8(uint8_t* value);
	bool readInt16(int16_t* value);
	bool readUint16(uint16_t* value);
	bool readInt32(int32_t* value);
	bool readUint32(uint32_t* value);
	bool readInt64(int64_t* value);
	bool readUint64(uint64_t* value);

	bool peekInt8(int8_t* value, size_t offset = 0);
	bool peekUint8(uint8_t* value, size_t offset = 0);
	bool peekInt16(int16_t* value, size_t offset = 0);
	bool peekUint16(uint16_t* value, size_t offset = 0);
	bool peekInt32(int32_t* value, size_t offset = 0);
	bool peekUint32(uint32_t* value, size_t offset = 0);
	bool peekInt64(int64_t* value, size_t offset = 0);
	bool peekUint64(uint64_t* value, size_t offset = 0);

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
