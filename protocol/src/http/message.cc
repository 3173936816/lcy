#include "lcy/protocol/src/http/message.h"

namespace lcy {
namespace protocol {
namespace http {

std::string VersionToString(version_type v)
{
	if ( v == version::HTTP_1_0 ) {
		return "HTTP/1.0";
	} else if ( v == version::HTTP_1_1 ) {
		return "HTTP/1.1";
	}
//	return "";
	return "HTTP/1.1";				// Default version 1.1
}

version_type StringToVersion(const std::string& v_str)
{
	if ( v_str == "HTTP/1.0" ) {
		return version::HTTP_1_0;
	} else if ( v_str == "HTTP/1.1" ) {
		return version::HTTP_1_1;
	}
//	return version::HTTP_ERR;
	return version::HTTP_1_1;		// Default version 1.1
}

//////////////////////////////////////////////////

Message::Message(proto_type type) :
    type_(type),
	version_(version::HTTP_ERR)
{
}

Message::~Message()
{
}

Message::proto_type Message::type() const
{
	return type_;
}

bool Message::isRequest() const
{
	return type_ == proto_type::REQUEST;
}

bool Message::isResponse() const
{
	return type_ == proto_type::RESPONSE;
}

const std::string& Message::body() const
{
	return body_;
}

void Message::setBody(std::string body)
{
	body_ = std::move(body);
}

version_type Message::version() const
{
	return version_;
}

void Message::setVersion(version_type v)
{
	version_ = v;
}

Message::header_map_type& Message::getHeadersRef()
{
	return headers_;
}

const Message::header_map_type& Message::getHeadersRef() const
{
	return headers_;
}

const std::string& Message::getHeader(const std::string& key,
							   		  const std::string& def) const
{
	auto kv_iter = headers_.find(key);
	if ( kv_iter != headers_.end() ) {
		return kv_iter->second;
	}
	return def;
}

void Message::removeHeader(const std::string& key)
{
	headers_.erase(key);
}

void Message::setHeader(std::string key, std::string value)
{
	headers_[std::move(key)] = std::move(value);
}

void Message::clear()
{
	body_ = {};
	version_ = version::HTTP_ERR;
	headers_.clear();
}

}   // namespace http
}   // namespace protocol
}	// namespace lcy 
