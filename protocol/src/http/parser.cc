#include "http_parser.h"
#include "http_request.h"
#include "http_response.h"

#include <sstream>
#include <algorithm>
#include <iostream>
#include <cctype>

namespace libserver {
namespace http {

class HttpParser::Impl {
public:
    enum Tag {
        AGREEMENT_LINE = 0,
        AGREEMENT_HEADER = 1,
        AGREEMENT_BODY = 2,
        AGREEMENT_READY = 3
    };

    Impl();
    ~Impl();

    void reset();
    HttpParser::RetCode parse(DynamicBuffer* buffer);
    std::shared_ptr<HttpMessage> get();

private:
    ssize_t ParseAgreementLine(const std::string& message); 
    ssize_t ParseAgreementHeader(const std::string& message); 
    ssize_t ParseAgreementBody(const std::string& message);
    
    bool ParseHeaderLine(const std::string& line, std::string& key, std::string& value);
    bool CaseInsensitiveCompare(const std::string& a, const std::string& b);

private:
    Tag tag_;
    std::shared_ptr<HttpMessage> http_message_;
    size_t content_length_;
    bool chunked_transfer_;
};

/////////////////////////////////////////////////////////////////////////////////////////////
HttpParser::Impl::Impl() :
    tag_(Tag::AGREEMENT_LINE),
    http_message_(nullptr),
    content_length_(0),
    chunked_transfer_(false)
{
}

HttpParser::Impl::~Impl()
{
}

void HttpParser::Impl::reset()
{
    tag_ = Tag::AGREEMENT_LINE;
    http_message_.reset();
    content_length_ = 0;
    chunked_transfer_ = false;
}

bool HttpParser::Impl::CaseInsensitiveCompare(const std::string& a, const std::string& b)
{
    if (a.length() != b.length()) {
        return false;
    }
    for (size_t i = 0; i < a.length(); ++i) {
        if (std::tolower(a[i]) != std::tolower(b[i])) {
            return false;
        }
    }
    return true;
}

HttpParser::RetCode HttpParser::Impl::parse(DynamicBuffer* buffer)
{
    std::string message(buffer->readBegin(), buffer->dataBytes());
    ssize_t nparse = 0;

    switch(tag_) {
        case Tag::AGREEMENT_LINE: {
            nparse = ParseAgreementLine(message);
            if (nparse < 0) return HttpParser::ERROR;
            if (nparse == 0) return HttpParser::WAITING_DATA;
            buffer->read(nparse);
            tag_ = Tag::AGREEMENT_HEADER;
            break;
        }
        case Tag::AGREEMENT_HEADER: {
            nparse = ParseAgreementHeader(message);
            if (nparse < 0) return HttpParser::ERROR;
            if (nparse == 0) return HttpParser::WAITING_DATA;
            buffer->read(nparse);
            if (chunked_transfer_ || content_length_ > 0) {
                tag_ = Tag::AGREEMENT_BODY;
            } else {
                tag_ = Tag::AGREEMENT_READY;
                return HttpParser::READY;
            }
            break;
        }
        case Tag::AGREEMENT_BODY: {
            nparse = ParseAgreementBody(message);
            if (nparse < 0) return HttpParser::ERROR;
            if (nparse > 0) {
                buffer->read(nparse);
                tag_ = Tag::AGREEMENT_READY;
                return HttpParser::READY;
            }
            if (!chunked_transfer_ && content_length_ == 0) {
                tag_ = Tag::AGREEMENT_READY;
                return HttpParser::READY;
            }
            return HttpParser::WAITING_DATA;
        }
        case Tag::AGREEMENT_READY: {
            return HttpParser::READY;
        }
    }
    
    if (nparse > 0 && tag_ != Tag::AGREEMENT_READY) {
        return parse(buffer);
    }
    
    return HttpParser::WAITING_DATA;
}

std::shared_ptr<HttpMessage> HttpParser::Impl::get()
{
    auto result = http_message_;
    reset();
    return result;
}

ssize_t HttpParser::Impl::ParseAgreementLine(const std::string& message)
{
    size_t crlf = message.find("\r\n");
    if (crlf == std::string::npos) return 0;

    std::string agreement_line = message.substr(0, crlf);
    
    std::istringstream iss(agreement_line);
    std::vector<std::string> tokens;
    std::string token;
    
    while (iss >> token) tokens.push_back(token);
    
    if (tokens.size() < 3) return -1;
    
    RequestMethod method = StringToRequestMethod(tokens[0]);
    if (RequestMethod::INVALID_METHOD != method) {
        http_message_.reset(new HttpRequest());
        HttpRequest* request = static_cast<HttpRequest*>(http_message_.get());
        request->setMethod(method);
        request->setUrl(tokens[1]);
        request->setVersion(tokens[2]);
    } else {
        http_message_.reset(new HttpResponse());
        HttpResponse* response = static_cast<HttpResponse*>(http_message_.get());
        response->setVersion(tokens[0]);
        response->setState(StringToResponseState(tokens[1]));
    }

    return crlf + 2;
}

ssize_t HttpParser::Impl::ParseAgreementHeader(const std::string& message)
{
    if ( message.size() >= 2 &&
         message.substr(0, 2) == "\r\n" ) {
        return 2;
    }

    size_t dcrlf = message.find("\r\n\r\n");
    if (dcrlf == std::string::npos) {
        return 0;
    }

    std::string headers_block = message.substr(0, dcrlf);
    size_t pos = 0;
    size_t end = 0;
    
    if (headers_block.empty() || headers_block == "\r\n") {
        return dcrlf + 4;
    }
    
    while ((end = headers_block.find("\r\n", pos)) != std::string::npos) {
        std::string line = headers_block.substr(pos, end - pos);
        pos = end + 2;
        
        if (line.empty()) continue;
        
        std::string key, value;
        if (!ParseHeaderLine(line, key, value)) return -1;
        
        http_message_->appendHeader(key, value);
        
        if (CaseInsensitiveCompare(key, "Content-Length")) {
            try {
                content_length_ = std::stoul(value);
            } catch (...) {
                return -1;
            }
        }
        
        if (CaseInsensitiveCompare(key, "Transfer-Encoding")) {
            std::string lower_value = value;
            std::transform(lower_value.begin(), lower_value.end(), lower_value.begin(), 
                         [](unsigned char c) { return std::tolower(c); });
            chunked_transfer_ = (lower_value.find("chunked") != std::string::npos);
        }
    }
    
    return dcrlf + 4;
}

ssize_t HttpParser::Impl::ParseAgreementBody(const std::string& message)
{
    if (!chunked_transfer_ && content_length_ == 0) return 0;
    
    if (chunked_transfer_) {
        size_t pos = 0;
        std::string body;
        bool finished = false;
        
        while (pos < message.size() && !finished) {
            size_t crlf = message.find("\r\n", pos);
            if (crlf == std::string::npos) return 0;
            
            std::string chunk_size_str = message.substr(pos, crlf - pos);
            size_t chunk_size;
            try {
                chunk_size = std::stoul(chunk_size_str, nullptr, 16);
            } catch (...) {
                return -1;
            }
            
            if (chunk_size == 0) {
                pos = crlf + 4;
                finished = true;
                break;
            }
            
            if (message.size() < crlf + 2 + chunk_size + 2) return 0;
            
            body += message.substr(crlf + 2, chunk_size);
            pos = crlf + 2 + chunk_size + 2;
        }
        
        if (finished) {
            if (dynamic_cast<HttpRequest*>(http_message_.get())) {
                static_cast<HttpRequest*>(http_message_.get())->setBody(body);
            } else {
                static_cast<HttpResponse*>(http_message_.get())->setBody(body);
            }
            return pos;
        }
        return 0;
    } else {
        if (message.size() < content_length_) return 0;
        
        std::string body = message.substr(0, content_length_);
        if (dynamic_cast<HttpRequest*>(http_message_.get())) {
            static_cast<HttpRequest*>(http_message_.get())->setBody(body);
        } else {
            static_cast<HttpResponse*>(http_message_.get())->setBody(body);
        }
        
        return content_length_;
    }
}

bool HttpParser::Impl::ParseHeaderLine(const std::string& line, std::string& key, std::string& value)
{
    size_t colon = line.find(':');
    if (colon == std::string::npos) return false;
    
    key = line.substr(0, colon);
    value = line.substr(colon + 1);
    
    size_t key_start = key.find_first_not_of(" \t");
    size_t key_end = key.find_last_not_of(" \t");
    if (key_start != std::string::npos && key_end != std::string::npos) {
        key = key.substr(key_start, key_end - key_start + 1);
    }
    
    size_t value_start = value.find_first_not_of(" \t");
    size_t value_end = value.find_last_not_of(" \t");
    if (value_start != std::string::npos && value_end != std::string::npos) {
        value = value.substr(value_start, value_end - value_start + 1);
    }
    
    return !key.empty();
}

////////////////////////////////////////////////////////////////////////////
HttpParser::HttpParser() :
    pImpl_(new Impl())
{
}

HttpParser::~HttpParser()
{
}

void HttpParser::reset()
{
    pImpl_->reset();
}

HttpParser::RetCode HttpParser::parse(DynamicBuffer* buffer)
{
    return pImpl_->parse(buffer);
}

std::shared_ptr<HttpMessage> HttpParser::get()
{
    return pImpl_->get();
}

}   // namespace http
}   // namespace libserver