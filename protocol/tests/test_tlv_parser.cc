#include "../protocol.hpp"

#include <string.h>
#include <arpa/inet.h>
#include <iostream>

int main() {
	uint16_t type = 32;
	std::string value = "hello world";
	uint32_t length = value.length();

	uint16_t encode_type = htons(type);
	uint32_t encode_length = htonl(length);

	std::string str;
	str.resize(sizeof(uint16_t) + sizeof(uint16_t));

	::memcpy(&str[0], &encode_type, sizeof(uint16_t));
	::memcpy(&str[0] + sizeof(uint16_t), &encode_length, sizeof(uint16_t));
	
	
	lcy::protocol::tlv::Message msg;
	lcy::protocol::tlv::Parser parser;

	lcy::protocol::tlv::Parser::RetCode retcode = parser.parse(&str[0], str.length(), msg);
	if ( retcode == lcy::protocol::tlv::Parser::RetCode::WAITING_DATA ) {
		std::cout << "waitting data" << std::endl;
	} else {
		std::cout << msg.type() << std::endl;
		std::cout << msg.length() << std::endl;
		std::cout << msg.value() << std::endl;
		std::cout << parser.nparse() << std::endl;
	}


	std::cout << std::endl;
 	/////////////////////////////////////////////////////////

	str.resize(6);
	::memcpy(&str[0] + 4, (char*)&encode_length + 2, sizeof(uint16_t));
	str += value.substr(0, 8);
	str += value.substr(8);

	retcode = parser.parse(&str[0], str.length(), msg);
	if ( retcode == lcy::protocol::tlv::Parser::RetCode::WAITING_DATA ) {
		std::cout << "waitting data" << std::endl;
	} else {
		std::cout << msg.type() << std::endl;
		std::cout << msg.length() << std::endl;
		std::cout << msg.value() << std::endl;
		std::cout << parser.nparse() << std::endl;
	}

	return 0;
}
