#include "lcy/protocol/src/http/request.h"

#include <sstream>

namespace lcy {
namespace protocol {
namespace http {

std::string MethodToString(method_type method)
{
#define XX(_code, _method, _desc)  \
    if ( (int)method == _code ) { return #_method; }
    HTTP_METHOD_MAP(XX);
#undef XX
    return "";
}

method_type StringToMethod(const std::string& method_str)
{
#define XX(_code, _method, _desc)   \
    if ( #_method == method_str ) { return method::_method; }
    HTTP_METHOD_MAP(XX);
#undef XX
    return method::INVALID;
}

///////////////////////////////////////////////////////

Request::Request() :
	Message(Message::proto_type::REQUEST),
	method_(method::INVALID)
{
}

Request::~Request()
{
}

method_type Request::method() const
{
	return method_;
}

void Request::setMethod(method_type method)
{
	method_ = method;
}

const std::string& Request::uri() const
{
	return uri_;
}

void Request::setUri(std::string uri)
{
	uri_ = std::move(uri);
}

void Request::clear()
{
	Message::clear();
	method_ = method::INVALID;
	uri_ = {};
}

std::string Request::dump() const
{
	std::ostringstream oss;
	oss << MethodToString(method_) << " "
		<< uri_ << " "
		<< VersionToString(version()) 
		<< "\r\n";

	for ( auto& kv : getHeadersRef() ) {
		oss << kv.first << ": " << kv.second << "\r\n";
	}

	oss << "\r\n";
	oss << body();

	return oss.str();
}

}   // namespace http
}   // namespace protocol
}	// namespace lcy 
