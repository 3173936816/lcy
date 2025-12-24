#ifndef __LCY_RPC_CONTEXT_HPP__
#define __LCY_RPC_CONTEXT_HPP__

#include <memory>

#include "lcy/rpc/controller.h"

namespace lcy {
namespace rpc {

template <class request_type, class response_type> 
class Context {
public:
	template<typename... Args>
    friend std::shared_ptr<Context> std::make_shared(Args&&... args);

	~Context();

	Controller* controller();
	request_type* request();
	response_type* response();

private:
	Context();
	Context(const Context&);
	Context& operator=(const Context&);

private:
	Controller controller_;
	request_type request_;
	response_type response_;
};

template <class request_type, class response_type> 
std::shared_ptr<Context<request_type, response_type> > NewContext();

}	// namespace rpc
}	// namespace lcy

#include "lcy/rpc/context.ipp"

#endif // __LCY_RPC_CONTEXT_HPP__
