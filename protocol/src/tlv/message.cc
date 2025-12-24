#include "lcy/protocol/src/tlv/message.h"
#include "lcy/protocol/src/tlv/variant_encode.h"

#include <string.h>
#include <arpa/inet.h>

namespace lcy {
namespace protocol {
namespace tlv {

Message::Message() :
	type_(0),
	length_(0)
{
}

Message::~Message()
{
}

uint16_t Message::type() const
{
	return type_;
}

uint32_t Message::length() const
{
	return length_;
}

const std::string& Message::value() const
{
	return value_;
}

void Message::setType(uint16_t type)
{
	type_ = type;
}

void Message::setLength(uint32_t length)
{
	length_ = length;
}

void Message::setValue(std::string value)
{
	value_ = std::move(value);
}

std::string Message::dump() const
{
//	if ( value_.size() < length_ ) return "";
//
//	size_t encode_len = 0;
//	char buf[BUFLEN_UINT16 + BUFLEN_UINT32];
//
//	encode_len = VariantEncodeUint16(type_, buf, sizeof(buf));
//	encode_len += VariantEncodeUint32(length_,
//									  buf + encode_len,
//									  sizeof(buf) - encode_len);
//	std::string tmp;
//	tmp += std::string(buf, encode_len);
//	tmp += std::string(value_.c_str(), length_);
//
//	return tmp;

	if ( value_.size() < length_ ) return "";
	
	uint16_t type_tmp = htons(type_);
	uint32_t length_tmp = htonl(length_);
	
	std::string tmp;
	tmp.resize(sizeof(type_tmp) + sizeof(length_tmp));
	::memcpy(&tmp[0], &type_tmp, sizeof(type_tmp));
	::memcpy(&tmp[0] + sizeof(type_tmp), &length_tmp, sizeof(length_tmp));

	tmp += std::string(value_.c_str(), length_);

	return tmp;
}

}	// namespace tlv
}	// namespace protocol
}	// namespace lcy
