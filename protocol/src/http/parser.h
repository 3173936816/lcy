#ifndef __LIBSERVER_HTTP_PARSER_H__
#define __LIBSERVER_HTTP_PARSER_H__

#include <memory>

namespace libserver {
namespace http {

class HttpMessage;
class HttpRequest;
class HttpResponse;

class HttpParser {
public:
    enum RetCode {
        READY,              // http protocol is ready
        WAITING_DATA,       // http protocol is incomplete
        ERROR               // an error occurred
    };

    HttpParser();
    ~HttpParser();

    void reset();
    RetCode parse(DynamicBuffer* buffer);
    std::shared_ptr<HttpMessage> get();

private:
    CORNERSTONES_NONCOPYABLE(HttpParser)
   
    class Impl;
    std::unique_ptr<Impl> pImpl_;
};

}   // namespace http
}   // namespace libserver

#endif  // __LIBSERVER_HTTP_PARSER_H__
