#include "lcy/protocol/src/http/response.h"

#include <sstream>

namespace lcy {
namespace protocol {
namespace http {

std::string StateToString(state_type state)
{
#define XX(_code, _state, _desc) \
    if ( _code == (int)state ) { return #_state; }
    HTTP_STATUS_MAP(XX)
#undef XX
    return "";
}

state_type StringToState(const std::string& state_str)
{
#define XX(_code, _state, _desc) \
    if ( #_state == state_str ) { return state::_state; }
    HTTP_STATUS_MAP(XX)
#undef XX
    return state::INVALID;
}

std::string StateToDesc(state_type state)
{
#define XX(_code, _state, _desc)    \
    if ( _code == (int)state ) {           \
        return #_desc;  \
    }
    HTTP_STATUS_MAP(XX)
#undef XX
    return "";
}

Response::Response() :
    Message(Message::proto_type::RESPONSE),
    state_(state::INVALID)
{
}

Response::~Response()
{
}

state_type Response::state() const
{
	return state_;
}

void Response::setState(state_type state)
{
	state_ = state;
}

void Response::clear()
{
	Message::clear();
	
	state_ = state::INVALID;
}

std::string Response::dump() const
{
	std::ostringstream oss;

	oss << VersionToString(version()) << " "
		<< (int)state_ << " "
		<< StateToDesc(state_)
		<< "\r\n";

	for ( auto& kv_iter : getHeadersRef() ) {
		oss << kv_iter.first << ": " << kv_iter.second << "\r\n";
	}

	oss << "\r\n";
	oss << body();

	return oss.str();
}

}   // namespace http
}   // namespace protocol
}	// namespace lcy
