#ifndef __LCY_PROTOCOL_HTTP_SERVLET_H__
#define __LCY_PROTOCOL_HTTP_SERVLET_H__ 

#include <string>
#include <memory>
#include <functional>

#include <unordered_map>

namespace lcy {
namespace protocol {
namespace http {

class Request;
class Response;

class Servlet {
public:
    typedef std::function<
				void (const Request&, Response&)
			> function_type;

    Servlet();
    ~Servlet();

    void setFunction(const std::string& url, function_type func);
    void removeFunction(const std::string& url);
    function_type getFunction(const std::string& url) const;

private:
	typedef std::unordered_map<
				std::string, 
				function_type
			> url_func_umap_type;

	url_func_umap_type url_func_umap_;
};

}   // namespace http
}   // namespace protocol
}	// namespace lcy

#endif // __LCY_PROTOCOL_HTTP_SERVLET_H__
