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

void Channel::CallMethod(const ::google::protobuf::MethodDescriptor* method,
                		 ::google::protobuf::RpcController* controller,
						 const ::google::protobuf::Message* request,
                    	 ::google::protobuf::Message* response,
						 ::google::protobuf::Closure* done)
{
	lcy::rpc::Request rpc_request;
	rpc_request.set_id(id_allocator_++);
}

}	// namespace rpc
}	// namespace lcy
