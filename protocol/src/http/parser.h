#ifndef __LCY_PROTOCOL_HTTP_PARSER_H__
#define __LCY_PROTOCOL_HTTP_PARSER_H__

#include <memory>

namespace lcy {
namespace protocol {
namespace http {

class Message;
class Request;
class Response;

class Parser {
public:
    enum RetCode {
        READY,              // http protocol is ready
        WAITING_DATA,       // http protocol is incomplete
        ERROR               // an error occurred
    };

    Parser();
    ~Parser();

    void reset();
	size_t nparse() const;
    RetCode parse(const void* data, size_t len, Message& msg);

private:
	Parser(const Parser&);
	Parser& operator=(const Parser&);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl_;
};

}   // namespace http
}	// namespace protocol
}   // namespace lcy

#endif  // __LCY_PROTOCOL_HTTP_PARSER_H__
