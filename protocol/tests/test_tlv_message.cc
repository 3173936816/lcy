#include "../protocol.hpp"
#include <iostream>

int main() {

	std::string value = "hello world";

	lcy::protocol::tlv::Message msg;
	msg.setType(20);
	msg.setLength(value.length());
	msg.setValue(std::move(value));

	std::string tmp = msg.dump();
	std::cout << "len = " << tmp.length() << '\n' << tmp << std::endl;

	for ( size_t i = 0; i < tmp.length(); ++i ) {
		printf("0x%x\n", (uint8_t)tmp[i]);
	}

	return 0;
} 
