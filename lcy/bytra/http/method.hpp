#ifndef __LCY_BYTRA_HTTP_METHOD_HPP__
#define __LCY_BYTRA_HTTP_METHOD_HPP__

namespace lcy
{
namespace bytra
{
namespace http
{

#define LCY_BYTRA_HTTP_METHOD_MAP(XX)   \
  XX(0,  delete,      DELETE)           \
  XX(1,  get,         GET)              \
  XX(2,  head,        HEAD)             \
  XX(3,  post,        POST)             \
  XX(4,  put,         PUT)              \
  XX(5,  connect,     CONNECT)          \
  XX(6,  options,     OPTIONS)          \
  XX(7,  trace,       TRACE)            \
  XX(8,  copy,        COPY)             \
  XX(9,  lock,        LOCK)             \
  XX(10, mkcol,       MKCOL)            \
  XX(11, move,        MOVE)             \
  XX(12, propfind,    PROPFIND)         \
  XX(13, proppatch,   PROPPATCH)        \
  XX(14, search,      SEARCH)           \
  XX(15, unlock,      UNLOCK)           \
  XX(16, bind,        BIND)             \
  XX(17, rebind,      REBIND)           \
  XX(18, unbind,      UNBIND)           \
  XX(19, acl,         ACL)              \
  XX(20, report,      REPORT)           \
  XX(21, mkactivity,  MKACTIVITY)       \
  XX(22, checkout,    CHECKOUT)         \
  XX(23, merge,       MERGE)            \
  XX(24, msearch,     M-SEARCH)         \
  XX(25, notify,      NOTIFY)           \
  XX(26, subscribe,   SUBSCRIBE)        \
  XX(27, unsubscribe, UNSUBSCRIBE)      \
  XX(28, patch,       PATCH)            \
  XX(29, purge,       PURGE)            \
  XX(30, mkcalendar,  MKCALENDAR)       \
  XX(31, link,        LINK)             \
  XX(32, unlink,      UNLINK)           \
  XX(33, source,      SOURCE)           \
  XX(34, unknown,     UNKNOWN)          \


enum method
{
#define XX(xx_num, xx_name, xx_string)  \
      xx_name = xx_num,
    LCY_BYTRA_HTTP_METHOD_MAP(XX)
#undef XX
};

inline std::string method_to_string(method m)
{
    switch ( m ) {
      #define XX(xx_num, xx_name, xx_string)  \
            case method::xx_name : { return xx_string; break; }
          LCY_BYTRA_HTTP_METHOD_MAP(XX)
      #undef XX
      default : {
          return "Unknown";
      }
    }
}

inline method method_from_string(const std::string& s)
{
#define XX(xx_num, xx_name, xx_string)  \
        if ( s == #xx_string ) { return method::xx_name; }
    LCY_BYTRA_HTTP_METHOD_MAP(XX)
#undef XX
    return method::unknown;
}

}   // namespace http
}   // namespace bytra
}   // namespace lcy

#endif  // __LCY_BYTRA_HTTP_METHOD_HPP__