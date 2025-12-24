#ifndef __LCY_PROTOCOL_HTTP_MESSAGE_H__
#define __LCY_PROTOCOL_HTTP_MESSAGE_H__

#include <string>
#include <unordered_map>

namespace lcy {
namespace protocol {
namespace http {

typedef 
enum class version : 
	uint8_t 
{
	HTTP_ERR = 0x00,
	HTTP_1_0 = 0x10,
	HTTP_1_1 = 0x11,
} 
version_type;

std::string VersionToString(version_type v);
version_type StringToVersion(const std::string& v_str);

class Message {
public:
    typedef std::unordered_map<
				std::string, 
				std::string
			> header_map_type;

    enum proto_type {
        UNKNOWN,
        REQUEST,
        RESPONSE
    };

    Message(proto_type type);
    virtual ~Message();

	proto_type type() const;
	bool isRequest() const;
	bool isResponse() const;

	const std::string& body() const;
	void setBody(std::string body);	

	version_type version() const;
	void setVersion(version_type v);

	header_map_type& getHeadersRef();
	const header_map_type& getHeadersRef() const;
	const std::string& getHeader(const std::string& key, 
						  		 const std::string& def = "") const;
	void removeHeader(const std::string& key);
	void setHeader(std::string key, std::string value);

	virtual void clear();
    virtual std::string dump() const = 0;

private:
    proto_type type_;
    std::string body_;
	version_type version_;
    header_map_type headers_;
};

}   // namespace http
}   // namespace protocol
}	// namespace lcy 

#endif // __LCY_PROTOCOL_HTTP_MESSAGE_H__
