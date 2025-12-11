#include "protocol/http/parser.h"
#include "protocol/http/request.h"
#include "protocol/http/response.h"

#include <sstream>
#include <algorithm>
#include <iostream>
#include <cctype>

namespace lcy {
namespace protocol {
namespace http {

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
}

size_t Parser::Impl::nparse() const
{
	return nparse_;
}

RetCode Parser::Impl::parse(const void* data, size_t len, Message& msg)
{
	const char* data_ptr = (const char*)data;

	while ( tag_ != READY ) {
		size_t nparse = 0;
		Parser::RetCode retcode = Parser::RetCode::ERROR;

		switch (tag_) {
			case LINE : {
				if ( msg.type() == Message::proto_type::REQUEST )
					retcode = parseRequestLine(data_ptr, len, msg, nparse);	
				else if ( msg.type() == Message::proto_type::RESPNSE )
					retcode = parseResponseLine(data_ptr, len, msg, nparse);	
				else
					return Parser::RetCode::ERROR;
			
				if ( retcode == Parser::RetCode::READY ) {
					already_parse_ += nparse;
					tag_ = Tag::HEADER;

				} else {
					return retcode;
				}
					
				break;
			}
		
			case HEADER : {
				retcode = parseHeader(data_ptr + already_parse_, 
									  len - already_parse_, msg, nparse);
				
				if ( retcode == Parser::RetCode::READY ) {
					already_parse_ += nparse;
					tag_ = Tag::BODY;
					
					content_length_ = std::atoi(msg.getHeader("Content-Length", "0"));
					std::string def = "No Transfer-Encoding";
					if ( msg.getHeader("Transfer-Encoding", def) == "chunked" )
						chunked_transfer_ = true;

				} else {
					return retcode;
				}
					
				break;
			}

			case BODY : {
				retcode = parseBody(data_ptr + already_parse_, 
									len - already_parse_, 
									msg, nparse, content_length_, chunked_transfer_);

				if ( retcode == Parser::RetCode::READY ) {
					already_parse_ += nparse;
					tag_ = Tag::READY;

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

static bool isValidURL(const std::string& url) {
    std::regex url_regex(R"(^(https?://)\S+$)");
    return std::regex_match(url, url_regex);
}

static bool isValidHTTPVersion(const std::string& version) {
    std::regex version_regex(R"(^HTTP/\d+\.\d+$)");
    return std::regex_match(version, version_regex);
}

static Parser::RetCode requestLineSplit(const char* data_ptr, size_t len, Request& request)
{
	std::string method, url, version;
	
	int count = 0;
	const char* start_ptr = data_ptr;
	for ( size_t i = 0; i < len; ++i ) {
		if ( data_ptr[i] == ' ' ) {
			++count;

			std::string tmp = std::string(start_ptr, data_ptr + i - start_ptr);
			if ( count == 1 ) method = std::move(tmp);
			else if ( count == 2 ) url = std::move(tmp);
			else return Parser::RetCode::ERROR;

			start_ptr = data_ptr + i + 1;
		}
	}

	if ( count != 2 || start_ptr >= data_ptr + len )
		return Parser::RetCode::ERROR;

	version = std::string(start_ptr, data_ptr + len - start_ptr);

	if ( !isValidURL(url) || !isValidHTTPVersion(version) )
		return Parser::RetCode::ERROR;
	
	request.setMethod(std::move(method));
	request.setUrl(std::move(url));
	request.setVersion(StringToVersion(std::move(version)));

	return Parser::RetCode::READY;
}

static Parser::RetCode parseRequestLine(const char* data_ptr, size_t len, 
										Message& msg, size_t& nparse)
{
	Request* request = (Request*)&msg;

	for ( int i = 0, j = 1; j < len; ++i, ++j ) {
		if ( data_ptr[i] = '\r' && data_ptr[j] = '\n' ) {
			return requestLineSplit(data_ptr, i, *request);
		}
	}

	return Parser::RetCode::WAITING_DATA;
}

static Parser::RetCode parseResponseLine(const char* data_ptr, size_t len, 
										 Message& msg, size_t& nparse)
{
}

static Parser::RetCode parseHeader(const char* data_ptr, size_t len, 
								   Message& msg, size_t& nparse)
{
}

static Parser::RetCode parseBody(const char* data_ptr, size_t len, Message& msg, 
								 size_t& nparse, size_t content_len, bool is_chunk)
{
}
