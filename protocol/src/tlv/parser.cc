#include "lcy/protocol/src/tlv/parser.h"
#include "lcy/protocol/src/tlv/message.h"

#include <string.h>
#include <arpa/inet.h>

namespace lcy {
namespace protocol {
namespace tlv {

static Parser::RetCode parseType(const char* data_ptr,
								 size_t len, 
								 Message& msg,
								 size_t& nparse)
{
	if ( len < sizeof(uint16_t) ) {
		return Parser::RetCode::WAITING_DATA; 
	}

	uint16_t type = 0;
	::memcpy(&type, data_ptr, sizeof(uint16_t));

	msg.setType(ntohs(type));
	nparse += sizeof(uint16_t);

	return Parser::RetCode::READY;
}

static Parser::RetCode parseLength(const char* data_ptr,
								   size_t len,
								   Message& msg,
								   size_t& nparse)
{
	if ( len < sizeof(uint32_t) ) {
		return Parser::RetCode::WAITING_DATA; 
	}

	uint32_t length = 0;
	::memcpy(&length, data_ptr, sizeof(uint32_t));

	msg.setLength(ntohl(length));
	nparse += sizeof(uint32_t);

	return Parser::RetCode::READY;
}

static Parser::RetCode parseValue(const char* data_ptr,
								  size_t len,
								  Message& msg,
								  size_t& nparse)
{
	uint32_t length = msg.length();

	if ( len < length ) {
		return Parser::RetCode::WAITING_DATA;
	}
	
	std::string value = std::string(data_ptr, length);
	msg.setValue(std::move(value));

	nparse += length;
	
	return Parser::RetCode::READY;
}

////////////////////////////////////////////////////////////

class Parser::Impl {
public:
    enum Tag {
        TYPE = 0,
        LENGTH = 1,
		VALUE = 2,
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
};

////////////////////////////////////////////////////////////

Parser::Impl::Impl() :
	tag_(Tag::TYPE),
	already_parse_(0)
{
}

Parser::Impl::~Impl()
{
}

void Parser::Impl::reset()
{
	tag_ = Tag::TYPE;
	already_parse_ = 0;
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
		
		switch ( tag_ ) {
			case Tag::TYPE : {
				retcode = parseType(data_ptr + already_parse_, 
									len - already_parse_, msg, nparse);
				
				if ( retcode == Parser::RetCode::READY ) {
					already_parse_ += nparse;
					tag_ = Tag::LENGTH;
				} else {
					return retcode;
				}

				break;
			}

			case Tag::LENGTH : {
				retcode = parseLength(data_ptr + already_parse_, 
									  len - already_parse_, msg, nparse);
				
				if ( retcode == Parser::RetCode::READY ) {
					already_parse_ += nparse;
					tag_ = Tag::VALUE;
				} else {
					return retcode;
				}

				break;
			}

			case Tag::VALUE : {
				retcode = parseValue(data_ptr + already_parse_, 
									 len - already_parse_, msg, nparse);
				
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

//////////////////////////////////////////////////////////////////////

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

}	// namespace tlv
}	// namespace protocol
}	// namespace lcy
