#include "lcy/protocol/src/tlv/variant_encode.h"

namespace lcy {
namespace protocol {
namespace tlv {

#define ZIGZAG_ENCODE(rettype, v)				\
	if ( v >= 0 ) return (rettype)(v << 1);		\
	else return (rettype)(((-v) << 1) - 1);		\


#define ZIGZAG_DECODE(rettype, v)					\
	if ( v & 1 ) return -(rettype)((v + 1) >> 1);	\
	else return (rettype)(v >> 1);					\


uint8_t ZigzagEncodeInt8(int8_t v)
{
	ZIGZAG_ENCODE(uint8_t, v)
}

int8_t ZigzagDecodeUint8(uint8_t v)
{
	ZIGZAG_DECODE(int8_t, v)
}

uint16_t ZigzagEncodeInt16(int16_t v)
{
	ZIGZAG_ENCODE(uint16_t, v)
}

int16_t ZigzagDecodeUint16(uint16_t v)
{
	ZIGZAG_DECODE(int16_t, v)
}

uint32_t ZigzagEncodeInt32(int32_t v)
{
	ZIGZAG_ENCODE(uint32_t, v)
}

int32_t ZigzagDecodeUint32(uint32_t v)
{
	ZIGZAG_DECODE(int32_t, v)
}

uint64_t ZigzagEncodeInt64(int64_t v)
{
	ZIGZAG_ENCODE(uint64_t, v)
}

int64_t ZigzagDecodeUint64(uint64_t v)
{
	ZIGZAG_DECODE(int64_t, v)
}

#undef ZIGZAG_ENCODE

#undef ZIGZAG_DECODE

////////////////////////////////////////////////////

#define VARIANT_ENCODE(min_len, v, buf, len)	\
	if ( len < min_len ) return 0;				\
												\
	size_t count = 0;							\
	while ( v > 0x7f ) {						\
		uint8_t byte = (v & 0x7f) | 0x80;		\
		((uint8_t*)buf)[count++] = byte;		\
												\
		v >>= 7;								\
	}											\
	((uint8_t*)buf)[count++] = v;				\
												\
	return count;								\


#define VARIANT_DECODE(type, max_len, v, buf, len)			\
	type tmp = 0;											\
															\
	size_t shift = 0;										\
	uint8_t* buf_ptr = (uint8_t*)buf;						\
	for ( size_t i = 0; i < max_len && i < len; ++i ) {		\
		tmp |= ((type)(buf_ptr[i] & 0x7f)) << shift;		\
															\
		shift += 7;											\
		if ( !(buf_ptr[i] & 0x80) ) {						\
			v = tmp;										\
			return true;									\
		}													\
	}														\
															\
	return false;											\


size_t VariantEncodeUint8(uint8_t v, void* buf, size_t len)
{
	VARIANT_ENCODE(BUFLEN_UINT8, v, buf, len)
}

bool VariantDecodeUint8(uint8_t& v, void* buf, size_t len)
{
	VARIANT_DECODE(uint8_t, BUFLEN_UINT8, v, buf, len)
}

size_t VariantEncodeUint16(uint16_t v, void* buf, size_t len)
{
	VARIANT_ENCODE(BUFLEN_UINT16, v, buf, len)
}

bool VariantDecodeUint16(uint16_t& v, void* buf, size_t len)
{
	VARIANT_DECODE(uint16_t, BUFLEN_UINT16, v, buf, len)
}

size_t VariantEncodeUint32(uint32_t v, void* buf, size_t len)
{
	VARIANT_ENCODE(BUFLEN_UINT32, v, buf, len)
}

bool VariantDecodeUint32(uint32_t& v, void* buf, size_t len)
{
	VARIANT_DECODE(uint32_t, BUFLEN_UINT32, v, buf, len)
}

size_t VariantEncodeUint64(uint64_t v, void* buf, size_t len)
{
	VARIANT_ENCODE(BUFLEN_UINT64, v, buf, len)
}

bool VariantDecodeUint64(uint64_t& v, void* buf, size_t len)
{
	VARIANT_DECODE(uint64_t, BUFLEN_UINT64, v, buf, len)
}

#undef VARIANT_ENCODE

#undef VARIANT_DECODE

}	// namespace tlv
}	// namespace protocol
}	// namespace lcy
