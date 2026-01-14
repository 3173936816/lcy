#include "lcy/protocol/src/http/servlet.h"
#include "lcy/protocol/src/http/request.h"
#include "lcy/protocol/src/http/response.h"

namespace lcy {
namespace protocol {
namespace http {

Servlet::Servlet()
{
}

Servlet::~Servlet()
{
}

void Servlet::setFunction(std::string uri, function_type func)
{
	url_func_umap_[std::move(uri)] = std::move(func);
}

void Servlet::removeFunction(const std::string& uri)
{
	url_func_umap_.erase(uri);
}

Servlet::function_type Servlet::getFunction(const std::string& uri) const
{
	auto kv_iter = url_func_umap_.find(uri);
	if ( kv_iter != url_func_umap_.end() )
		return kv_iter->second;
	return {};
}

}   // namespace http
}   // namespace protocol
}	// namespace lcy
