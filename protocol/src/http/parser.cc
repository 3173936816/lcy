#include "protocol/src/http/parser.h"
#include "protocol/src/http/request.h"
#include "protocol/src/http/response.h"

#include <sstream>
#include <algorithm>
#include <iostream>
#include <cctype>
#include <regex>

namespace lcy {
namespace protocol {
namespace http {

static bool isValidURI(const std::string& uri) {
	std::regex uri_regex(R"(^/\S*$)");
    return std::regex_match(uri, uri_regex);
}

static bool isValidHTTPVersion(const std::string& version) {
    std::regex version_regex(R"(^HTTP/\d+\.\d+$)");
    return std::regex_match(version, version_regex);
}

static bool lineSplit(const char* data_ptr, 
					  size_t len, 
					  std::string& field1, 
					  std::string& field2, 
					  std::string& field3)
{
	int count = 0;
	const char* start_ptr = data_ptr;
	for ( size_t i = 0; i < len; ++i ) {
		if ( data_ptr[i] == ' ' ) {
			++count;

			std::string tmp = std::string(start_ptr, data_ptr + i - start_ptr);
			if ( count == 1 ) field1 = std::move(tmp);
			else if ( count == 2 ) field2 = std::move(tmp);
			else return false;

			start_ptr = data_ptr + i + 1;
		}
	}
	field3 = std::string(start_ptr, data_ptr + len - start_ptr);

	if ( count != 2 || start_ptr >= data_ptr + len )
		return false;
	return true;
}

static void trim(std::string& str)
{
	size_t i = 0, j = 0;
	for ( ; i < str.length(); ++i ) {
		if ( str[i] != ' ' ) {
			break;
		}
	}

	for ( j = i; j < str.length(); ++j ) {
		if ( str[j] == ' ' ) {
			break;
		}
	}

	str = str.substr(i, j - i);
}

static bool headerSplit(const char* data_ptr, size_t len, std::string& key, std::string& value)
{
	size_t colon = 0;
	for ( ; colon < len; ++colon ) {
		if ( data_ptr[colon] == ':' )
			break;
	}
	if ( colon >= len )
		return false;

	key = std::string(data_ptr, colon);
	value = std::string(data_ptr + colon + 1, len - colon - 1);

	trim(key);
	trim(value);

	return true;
}

static Parser::RetCode parseRequestLine(const char* data_ptr, 
										size_t len, 
										Request& request)
{
	std::string method, uri, version;
	
	if ( lineSplit(data_ptr, len, method, uri, version) ) {
		if ( !isValidURI(uri) || !isValidHTTPVersion(version) )
			return Parser::RetCode::ERROR;

		method_type method_tmp = StringToMethod(method);
		if ( method_tmp == method::INVALID )
			return Parser::RetCode::ERROR;
		
		request.setMethod(method_tmp);
		request.setUri(std::move(uri));
		request.setVersion(StringToVersion(version));

		return Parser::RetCode::READY;
	}

	return Parser::RetCode::ERROR;
}

static Parser::RetCode parseResponseLine(const char* data_ptr, 
										  size_t len, 
										  Response& response)
{
	std::string version, state, description;
	
	if ( lineSplit(data_ptr, len, version, state, description) ) {
		if ( !isValidHTTPVersion(version) )
			return Parser::RetCode::ERROR;

		state_type tmp_state = (state_type)std::atoi(state.c_str());
		if ( StateToString(tmp_state) == "" )	// FIXME : Not elegant enough
			return Parser::RetCode::ERROR;
		
		response.setVersion(StringToVersion(version));
		response.setState(tmp_state);

		return Parser::RetCode::READY;
	}

	return Parser::RetCode::ERROR;
}

static const char* CRLF(const char* data_ptr, size_t len)
{
	int i = 0, j = 1;
	for ( ; j < len; ++i, ++j ) {
		if ( data_ptr[i] == '\r' && data_ptr[j] == '\n' )
			break;
	}
	
	if ( j >= len )
		return nullptr;

	return data_ptr + i;
}

////////////////////////////////////////////////////////////////////

static Parser::RetCode parseLine(const char* data_ptr,
								 size_t len, 
								 Message& msg,
								 size_t& nparse)
{
	const char* crlf = CRLF(data_ptr, len);

	if ( !crlf )
		return Parser::RetCode::WAITING_DATA;

	size_t data_len = crlf - data_ptr;
	Parser::RetCode retcode = Parser::RetCode::ERROR;
	if ( msg.type() == Message::proto_type::REQUEST )
		retcode = parseRequestLine(data_ptr, data_len, *(Request*)&msg);
	else if ( msg.type() == Message::proto_type::RESPONSE )
		retcode = parseResponseLine(data_ptr, data_len, *(Response*)&msg);

	if ( retcode == Parser::RetCode::READY )
		nparse = data_len + 2;

	return retcode;
}

static Parser::RetCode parseHeader(const char* data_ptr,
								   size_t len, 
								   Message& msg,
								   size_t& nparse)
{
	nparse = 0;

	while ( true ) {
		const char* header_ptr = data_ptr + nparse;
		size_t remaining_len = len - nparse;

		const char* crlf = CRLF(header_ptr, remaining_len);
		if ( !crlf )
			return Parser::RetCode::WAITING_DATA;

		if ( crlf == header_ptr ) {
			nparse += 2;
			return Parser::RetCode::READY;
		}

		std::string key, value;
		size_t header_len = crlf - header_ptr;
		if ( headerSplit(header_ptr, header_len, key, value) ) {
			nparse += header_len + 2;
			msg.setHeader(std::move(key), std::move(value));
		} else {
			return Parser::RetCode::ERROR;
		}
	}
}

static Parser::RetCode parseBody(const char* data_ptr,
								 size_t len, 
								 Message& msg, 
								 size_t& nparse,
								 size_t content_len, 
								 bool is_chunk,
								 std::string& body_buf)
{
	return Parser::RetCode::READY;
}

/////////////////////////////////////////////////////////////

class Parser::Impl {
public:
    enum Tag {
        LINE = 0,
        HEADER = 1,
        BODY = 2,
        READY = 3,
    };

    Impl();
    ~Impl();

    void reset();
	size_t nparse() const;
    RetCode parse(const void* data, size_t len, Message& msg);

private:
    Tag tag_;
	size_t already_parse_;
    size_t content_length_;
    bool chunked_transfer_;
	std::string body_buf_;
};

////////////////////////////////////////////////////////////

Parser::Impl::Impl() :
    tag_(Tag::LINE),
	already_parse_(0),
    content_length_(0),
    chunked_transfer_(false)
{
}

Parser::Impl::~Impl()
{
}

void Parser::Impl::reset()
{
    tag_ = Tag::LINE;
	already_parse_ = 0;
    content_length_ = 0;
    chunked_transfer_ = false;
	body_buf_ = {};
}

size_t Parser::Impl::nparse() const
{
	return already_parse_;
}

Parser::RetCode Parser::Impl::parse(const void* data, size_t len, Message& msg)
{
	const char* data_ptr = (const char*)data;

	while ( tag_ != Tag::READY ) {
		size_t nparse = 0;
		Parser::RetCode retcode = Parser::RetCode::ERROR;

		switch (tag_) {
			case Tag::LINE : {
				retcode = parseLine(data_ptr, len, msg, nparse);
					
				if ( retcode == Parser::RetCode::READY ) {
					already_parse_ += nparse;
					tag_ = Tag::HEADER;

				} else {
					return retcode;
				}
					
				break;
			}
		
			case Tag::HEADER : {
				retcode = parseHeader(data_ptr + already_parse_, 
									  len - already_parse_, msg, nparse);
				
				if ( retcode == Parser::RetCode::READY ) {
					already_parse_ += nparse;
					tag_ = Tag::BODY;
					
					content_length_ = std::atoi(msg.getHeader("Content-Length", "0").c_str());
					std::string def = "No Transfer-Encoding";
					if ( msg.getHeader("Transfer-Encoding", def) == "chunked" )
						chunked_transfer_ = true;

					body_buf_.reserve(content_length_);

				} else if ( retcode == Parser::RetCode::WAITING_DATA ) {
					already_parse_ += nparse;
					return retcode;

				} else {
					return retcode;
				}
					
				break;
			}

			case Tag::BODY : {
				retcode = parseBody(data_ptr + already_parse_, 
									len - already_parse_, 
									msg, nparse, content_length_,
									chunked_transfer_, body_buf_);

				if ( retcode == Parser::RetCode::READY ) {
					already_parse_ += nparse;
					tag_ = Tag::READY;

				} else if ( retcode == Parser::RetCode::WAITING_DATA ) {
					already_parse_ += nparse;
					return retcode;
				
				} else {
					return retcode;
				}

				break;
			}
			
			default : {
				 /*
 				 *notify :  
 				 *	The control flow will never reach this point 
 				 */
			}
		}
	}
		
	return Parser::RetCode::READY;
}

///////////////////////////////////////////////////////////////////

Parser::Parser() :
	pImpl_(new Impl())
{
}

Parser::~Parser()
{
}

void Parser::reset()
{
	pImpl_->reset();
}

size_t Parser::nparse() const
{
	return pImpl_->nparse();
}

Parser::RetCode Parser::parse(const void* data, size_t len, Message& msg)
{
	return pImpl_->parse(data, len, msg);
}

}	// namespace http
}	// namespace protocol
}	// namespace lcy
