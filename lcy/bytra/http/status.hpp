#ifndef __LCY_BYTRA_HTTP_STATUS_HPP__
#define __LCY_BYTRA_HTTP_STATUS_HPP__

namespace lcy
{
namespace bytra
{
namespace http
{

#define LCY_BYTRA_HTTP_STATUS_MAP(XX)                                       \
  XX(0,   unknown,                         Unknown)                         \
  XX(100, continue_,                       Continue)                        \
  XX(101, switching_protocols,             Switching Protocols)             \
  XX(102, processing,                      Processing)                      \
  XX(200, ok,                              OK)                              \
  XX(201, created,                         Created)                         \
  XX(202, accepted,                        Accepted)                        \
  XX(203, non_authoritative_information,   Non-Authoritative Information)   \
  XX(204, no_content,                      No Content)                      \
  XX(205, reset_content,                   Reset Content)                   \
  XX(206, partial_content,                 Partial Content)                 \
  XX(207, multi_status,                    Multi-Status)                    \
  XX(208, already_reported,                Already Reported)                \
  XX(226, im_used,                         IM Used)                         \
  XX(300, multiple_choices,                Multiple Choices)                \
  XX(301, moved_permanently,               Moved Permanently)               \
  XX(302, found,                           Found)                           \
  XX(303, see_other,                       See Other)                       \
  XX(304, not_modified,                    Not Modified)                    \
  XX(305, use_proxy,                       Use Proxy)                       \
  XX(307, temporary_redirect,              Temporary Redirect)              \
  XX(308, permanent_redirect,              Permanent Redirect)              \
  XX(400, bad_request,                     Bad Request)                     \
  XX(401, unauthorized,                    Unauthorized)                    \
  XX(402, payment_required,                Payment Required)                \
  XX(403, forbidden,                       Forbidden)                       \
  XX(404, not_found,                       Not Found)                       \
  XX(405, method_not_allowed,              Method Not Allowed)              \
  XX(406, not_acceptable,                  Not Acceptable)                  \
  XX(407, proxy_authentication_required,   Proxy Authentication Required)   \
  XX(408, request_timeout,                 Request Timeout)                 \
  XX(409, conflict,                        Conflict)                        \
  XX(410, gone,                            Gone)                            \
  XX(411, length_required,                 Length Required)                 \
  XX(412, precondition_failed,             Precondition Failed)             \
  XX(413, payload_too_large,               Payload Too Large)               \
  XX(414, uri_too_long,                    URI Too Long)                    \
  XX(415, unsupported_media_type,          Unsupported Media Type)          \
  XX(416, range_not_satisfiable,           Range Not Satisfiable)           \
  XX(417, expectation_failed,              Expectation Failed)              \
  XX(421, misdirected_request,             Misdirected Request)             \
  XX(422, unprocessable_entity,            Unprocessable Entity)            \
  XX(423, locked,                          Locked)                          \
  XX(424, failed_dependency,               Failed Dependency)               \
  XX(426, upgrade_required,                Upgrade Required)                \
  XX(428, precondition_required,           Precondition Required)           \
  XX(429, too_many_requests,               Too Many Requests)               \
  XX(431, request_header_fields_too_large, Request Header Fields Too Large) \
  XX(451, unavailable_for_legal_reasons,   Unavailable For Legal Reasons)   \
  XX(500, internal_server_error,           Internal Server Error)           \
  XX(501, not_implemented,                 Not Implemented)                 \
  XX(502, bad_gateway,                     Bad Gateway)                     \
  XX(503, service_unavailable,             Service Unavailable)             \
  XX(504, gateway_timeout,                 Gateway Timeout)                 \
  XX(505, http_version_not_supported,      HTTP Version Not Supported)      \
  XX(506, variant_also_negotiates,         Variant Also Negotiates)         \
  XX(507, insufficient_storage,            Insufficient Storage)            \
  XX(508, loop_detected,                   Loop Detected)                   \
  XX(510, not_extended,                    Not Extended)                    \
  XX(511, network_authentication_required, Network Authentication Required) \


enum status
{
#define XX(xx_code, xx_name, xx_string) \
      xx_name = xx_code,
    LCY_BYTRA_HTTP_STATUS_MAP(XX)
#undef XX
};

inline std::string status_to_string(status s)
{
    return std::to_string(static_cast<int>(s));
}

inline status status_from_string(const std::string& s)
{
    int code = std::stoi(s);
    switch ( code ) {
    #define XX(xx_code, xx_name, xx_string)  \
            case xx_code : { return status::xx_name; break; }
        LCY_BYTRA_HTTP_STATUS_MAP(XX)
    #undef XX
        default : {
            return status::unknown;
        }
    }
}

inline std::string status_to_reason(status s,
  std::string default_value = "")
{
    switch ( s ) {
    #define XX(xx_code, xx_name, xx_string)  \
            case status::xx_name : { return #xx_string; break; }
        LCY_BYTRA_HTTP_STATUS_MAP(XX)
    #undef XX
        default : {
            return default_value;
        }
    }
}

}   // namespace http
}   // namespace bytra
}   // namespace lcy

#endif  // __LCY_BYTRA_HTTP_STATUS_HPP__