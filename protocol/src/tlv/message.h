#ifndef __LCY_PROTOCOL_TLV_MESSAGE_H__
#define __LCY_PROTOCOL_TLV_MESSAGE_H__

#include <stdint.h>
#include <string>

namespace lcy {
namespace protocol {
namespace tlv {

class Message {
public:
	Message();
	~Message();

	uint16_t type() const;
	uint32_t length() const;
	const std::string& value() const;

	void setType(uint16_t type);
	void setLength(uint32_t length);
	void setValue(std::string value);

	std::string dump() const;

private:
	uint16_t type_;
	uint32_t length_;
	std::string value_;
};

}	// namespace tlv
}	// namespace protocol
}	// namespace lcy

#endif // __LCY_PROTOCOL_TLV_MESSAGE_H__
