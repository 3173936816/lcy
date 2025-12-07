#include "protocol/src/http/servlet.h"
#include "protocol/src/http/request.h"
#include "protocol/src/http/response.h"

namespace lcy {
namespace protocol {
namespace http {

Servlet::Servlet()
{
}

Servlet::~Servlet()
{
}

void Servlet::setFunction(const std::string& url, function_type func)
{
	url_func_umap_[url] = std::move(func);
}

void Servlet::removeFunction(const std::string& url)
{
	url_func_umap_.erase(url);
}

Servlet::function_type Servlet::getFunction(const std::string& url) const
{
	auto kv_iter = url_func_umap_.find(url);
	if ( kv_iter != url_func_umap_.end() )
		return kv_iter->second;
	return {};
}

}   // namespace http
}   // namespace protocol
}	// namespace lcy
