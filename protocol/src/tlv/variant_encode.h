#ifndef __LCY_PROTOCOL_TLV_VARIANT_ENCODE_H__
#define __LCY_PROTOCOL_TLV_VARIANT_ENCODE_H__

#include <stdint.h>
#include <stddef.h>

namespace lcy {
namespace protocol {
namespace tlv {

uint8_t ZigzagEncodeInt8(int8_t v);
int8_t ZigzagDecodeUint8(uint8_t v);

uint16_t ZigzagEncodeInt16(int16_t v);
int16_t ZigzagDecodeUint16(uint16_t v);

uint32_t ZigzagEncodeInt32(int32_t v);
int32_t ZigzagDecodeUint32(uint32_t v);

uint64_t ZigzagEncodeInt64(int64_t v);
int64_t ZigzagDecodeUint64(uint64_t v);


constexpr size_t BUFLEN_UINT8  = (((sizeof(uint8_t) * 8) / 7) + 1);
constexpr size_t BUFLEN_UINT16 = (((sizeof(uint16_t) * 8) / 7) + 1);
constexpr size_t BUFLEN_UINT32 = (((sizeof(uint32_t) * 8) / 7) + 1);
constexpr size_t BUFLEN_UINT64 = (((sizeof(uint64_t) * 8) / 7) + 1);

size_t VariantEncodeUint8(uint8_t v, void* buf, size_t len);
bool VariantDecodeUint8(uint8_t& v, void* buf, size_t len);

size_t VariantEncodeUint16(uint16_t v, void* buf, size_t len);
bool VariantDecodeUint16(uint16_t& v, void* buf, size_t len);

size_t VariantEncodeUint32(uint32_t v, void* buf, size_t len);
bool VariantDecodeUint32(uint32_t& v, void* buf, size_t len);

size_t VariantEncodeUint64(uint64_t v, void* buf, size_t len);
bool VariantDecodeUint64(uint64_t& v, void* buf, size_t len);

}	// namespace tlv
}	// namespace protocol
}	// namespace lcy

#endif // __LCY_PROTOCOL_TLV_VARIANT_ENCODE_H__
