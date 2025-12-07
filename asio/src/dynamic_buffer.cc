#include "asio/src/dynamic_buffer.h"

#include <string.h>

namespace lcy {
namespace asio {

DynamicBuffer::DynamicBuffer(size_t expend_size) :
	read_index_(0),
	write_index_(0),
	expand_size_(expend_size),
	char_vector_(1)
{
}

DynamicBuffer::~DynamicBuffer()
{
}

char* DynamicBuffer::writeBegin()
{
	return &char_vector_.at(write_index_);
}

const char* DynamicBuffer::readBegin() const
{
	return &char_vector_.at(read_index_);
}

void DynamicBuffer::read(size_t size)
{
	read_index_ += size;

	if ( read_index_ == write_index_ ) {
		read_index_ = 0;
		write_index_ = 0;
	}
}

void DynamicBuffer::write(size_t size)
{
	write_index_ += size;
}

size_t DynamicBuffer::dataBytes() const
{
	return write_index_ - read_index_;
}

size_t DynamicBuffer::availableBytes() const
{
	return char_vector_.size() - write_index_;
}

size_t DynamicBuffer::uselessBytes() const
{
	return read_index_;
}

void DynamicBuffer::reserve(size_t size)
{
	if ( availableBytes() > size ) {
		return;
	}

	size_t new_size = char_vector_.size();
	if ( availableBytes() + uselessBytes() <= size ) {
		new_size = (size / expand_size_ + 1) 
				 * expand_size_ 
				 + char_vector_.size();
	}

	char_vector_.resize(new_size);
	::memmove(&char_vector_[0], readBegin(), dataBytes());

	write_index_ = dataBytes();
	read_index_ = 0;
}

std::string DynamicBuffer::readAllToString()
{
	std::string data_str = std::string(readBegin(), dataBytes());
	read(dataBytes());
	
	return data_str;
}

////////////////////////////////////////////////////

#define WRITE(value_type) 									\
	reserve(sizeof(value_type));							\
	::memcpy(writeBegin(), &value, sizeof(value_type));		\
	write(sizeof(value_type));								\

void DynamicBuffer::write8Bits(int8_t value)
{
	WRITE(int8_t)
}

void DynamicBuffer::write8Bits(uint8_t value)
{
	WRITE(uint8_t)
}

void DynamicBuffer::write16Bits(int16_t value)
{
	WRITE(int16_t)
}

void DynamicBuffer::write16Bits(uint16_t value)
{
	WRITE(uint16_t)
}

void DynamicBuffer::write32Bits(int32_t value)
{
	WRITE(int32_t)
}

void DynamicBuffer::write32Bits(uint32_t value)
{
	WRITE(uint32_t)
}

void DynamicBuffer::write64Bits(int64_t value)
{
	WRITE(int64_t)
}

void DynamicBuffer::write64Bits(uint64_t value)
{
	WRITE(uint64_t)
}

#undef WRITE

////////////////////////////////////////////////////////

#define READ(value_type)								\
	if ( dataBytes() < sizeof(value_type) ) {			\
		return false;									\
	}													\
	::memcpy(value, readBegin(), sizeof(value_type));	\
	read(sizeof(value_type));							\
	return true;										\

bool DynamicBuffer::read8Bits(int8_t* value)
{
	READ(int8_t)
}

bool DynamicBuffer::read8Bits(uint8_t* value)
{
	READ(uint8_t)
}

bool DynamicBuffer::read16Bits(int16_t* value)
{
	READ(int16_t)
}

bool DynamicBuffer::read16Bits(uint16_t* value)
{
	READ(uint16_t)
}

bool DynamicBuffer::read32Bits(int32_t* value)
{
	READ(int32_t)
}

bool DynamicBuffer::read32Bits(uint32_t* value)
{
	READ(uint32_t)
}

bool DynamicBuffer::read64Bits(int64_t* value)
{
	READ(int64_t)
}

bool DynamicBuffer::read64Bits(uint64_t* value)
{
	READ(uint64_t)
}

#undef READ

///////////////////////////////////////////////////////////

#define PEEK(value_type)										\
	if ( dataBytes() < sizeof(value_type) + offset ) {			\
		return false;											\
	}															\
	::memcpy(value, readBegin() + offset, sizeof(value_type));	\
	return true;												\


bool DynamicBuffer::peek8Bits(int8_t* value, size_t offset)
{
	PEEK(int8_t)
}

bool DynamicBuffer::peek8Bits(uint8_t* value, size_t offset)
{
	PEEK(uint8_t)
}

bool DynamicBuffer::peek16Bits(int16_t* value, size_t offset)
{
	PEEK(int16_t)
}

bool DynamicBuffer::peek16Bits(uint16_t* value, size_t offset)
{
	PEEK(uint16_t)
}

bool DynamicBuffer::peek32Bits(int32_t* value, size_t offset)
{
	PEEK(int32_t)
}

bool DynamicBuffer::peek32Bits(uint32_t* value, size_t offset)
{
	PEEK(uint32_t)
}

bool DynamicBuffer::peek64Bits(int64_t* value, size_t offset)
{
	PEEK(int64_t)
}

bool DynamicBuffer::peek64Bits(uint64_t* value, size_t offset)
{
	PEEK(uint64_t)
}

#undef PEEK

}	// namespace asio
}	// namespace lcy
