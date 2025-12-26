#include "lcy/rpc/src/channel.h"
#include "lcy/rpc/src/proto/rpc.pb.h"

namespace lcy {
namespace rpc {

Channel::Channel(lcy::asio::IOContext& ioc,
				 const std::string& ip,
				 uint16_t port) :
	id_allocator_(0),
	rpc_socket_(ioc)
{
}

Channel::~Channel()
{
	rpc_socket_.shutdown();
}

 /*
 *example :
 *
 * Service_Stub stub(new Channel(...));
 *
 * auto rpc_context = lcy::rpc::NewContext();
 * 	// Set your request
 * auto closure = lcy::rpc::NewClosure([rpc_context](){
 * 		if ( !rpc_context->controller()->IsFailed() ) {
 *			process(rpc_context->request());
 * 		} else {
 *			printf("%s", rpc_context->controller()->ErrorText().c_str());
 * 		}
 * });
 * 
 *
 * stub.userMethod(rpc_context->controller(), rpc_context->request(), rpc_context->response(), closure.get());
 *
 */

void Channel::CallMethod(const ::google::protobuf::MethodDescriptor* method,
                		 ::google::protobuf::RpcController* controller,
						 const ::google::protobuf::Message* request,
                    	 ::google::protobuf::Message* response,
						 ::google::protobuf::Closure* done)
{
	const ::google::protobuf::ServiceDescriptor* service = method->service();

	lcy::rpc::Request rpc_request;
	rpc_request.set_id(id_allocator_++);
	rpc_request.set_service_name(service->name());
	rpc_request.set_service_method(method->name());
	if ( request->SerializeToString(rpc_request_.mutable_arguments()) ) {
		controller->SetFailed("request serialize failed");
		done->run();
		return;
	}

	std::shared_ptr<std::string> data = std::make_shared<std::string>();
	if ( rpc_request->SerializeToString(data.get() ) {
		controller->SetFailed("rpc request serialize failed");
		done->run();
		return;
	}

}

}	// namespace rpc
}	// namespace lcy
