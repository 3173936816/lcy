#include "../protocol.hpp"

#include <iostream>

void test_zigzag() {
	int32_t i32 = -12;

	uint32_t ui32 = lcy::protocol::tlv::ZigzagEncodeInt32(i32);
	std::cout << "i32 = " << i32 << ", encode ui32 = " << ui32 << std::endl;
	std::cout << "ui32 decode = " << lcy::protocol::tlv::ZigzagDecodeUint32(ui32) << std::endl;
}

void test_variant() {
	
	uint32_t ui32 = 600;
	size_t len = 0;
	bool success = false;
	char buf[lcy::protocol::tlv::BUFLEN_UINT32];

	len = lcy::protocol::tlv::VariantEncodeUint32(ui32, buf, sizeof(buf));
	std::cout << "ui32 = " << ui32 << ", len = " << len << std::endl;

	uint32_t tmp = 0;
	success = lcy::protocol::tlv::VariantDecodeUint32(tmp, buf, len);
	std::cout << "success = " << success << ", encode = " << tmp << std::endl;

	printf("0x%x\n", (uint8_t)buf[0]);
	printf("0x%x\n", (uint8_t)buf[1]);
}

int main() {
	test_zigzag();

	test_variant();

	return 0;
}
