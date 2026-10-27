#ifndef __LCY_BYTRA_HTTP_METHOD_HPP__
#define __LCY_BYTRA_HTTP_METHOD_HPP__

namespace lcy
{
namespace bytra
{
namespace http
{

#define LCY_BYTRA_HTTP_METHOD_MAP(XX)   \
  XX(0,  DELETE,      DELETE)           \
  XX(1,  GET,         GET)              \
  XX(2,  HEAD,        HEAD)             \
  XX(3,  POST,        POST)             \
  XX(4,  PUT,         PUT)              \
  XX(5,  CONNECT,     CONNECT)          \
  XX(6,  OPTIONS,     OPTIONS)          \
  XX(7,  TRACE,       TRACE)            \
  XX(8,  COPY,        COPY)             \
  XX(9,  LOCK,        LOCK)             \
  XX(10, MKCOL,       MKCOL)            \
  XX(11, MOVE,        MOVE)             \
  XX(12, PROPFIND,    PROPFIND)         \
  XX(13, PROPPATCH,   PROPPATCH)        \
  XX(14, SEARCH,      SEARCH)           \
  XX(15, UNLOCK,      UNLOCK)           \
  XX(16, BIND,        BIND)             \
  XX(17, REBIND,      REBIND)           \
  XX(18, UNBIND,      UNBIND)           \
  XX(19, ACL,         ACL)              \
  XX(20, REPORT,      REPORT)           \
  XX(21, MKACTIVITY,  MKACTIVITY)       \
  XX(22, CHECKOUT,    CHECKOUT)         \
  XX(23, MERGE,       MERGE)            \
  XX(24, MSEARCH,     M-SEARCH)         \
  XX(25, NOTIFY,      NOTIFY)           \
  XX(26, SUBSCRIBE,   SUBSCRIBE)        \
  XX(27, UNSUBSCRIBE, UNSUBSCRIBE)      \
  XX(28, PATCH,       PATCH)            \
  XX(29, PURGE,       PURGE)            \
  XX(30, MKCALENDAR,  MKCALENDAR)       \
  XX(31, LINK,        LINK)             \
  XX(32, UNLINK,      UNLINK)           \
  XX(33, SOURCE,      SOURCE)           \

}   // namespace http
}   // namespace bytra
}   // namespace lcy

#endif  // __LCY_BYTRA_HTTP_METHOD_HPP__