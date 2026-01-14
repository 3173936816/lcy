#include "lcy/rpc/src/channel.h"
#include "lcy/rpc/src/rpc.pb.h"

#include <iostream>

namespace lcy {
namespace rpc {

MethodInfo::MethodInfo() :
	controller_(nullptr),
	request_(nullptr),
	response_(nullptr),
	done_(nullptr)
{
}

MethodInfo::MethodInfo(::google::protobuf::RpcController* controller,
					   const ::google::protobuf::Message* request,
		   			   ::google::protobuf::Message* response,
		   			   ::google::protobuf::Closure* done) :
	controller_(controller),
	request_(request),
	response_(response),
	done_(done)
{
}

MethodInfo::~MethodInfo()
{
}

//////////////////////////////////////////

Channel::Channel(lcy::asio::IOContext& ioc,
				 const std::string& ip,
				 uint16_t port) :
	is_connected_(false),
	id_allocator_(0),
	conn_(new details::Connection(ioc)),
	id_method_umap_()
{
	conn_->setConnectOp(std::bind(&Channel::onConnectOp,
			this, std::placeholders::_1));
	conn_->setMessageOp(std::bind(&Channel::onMessageOp,
			this, std::placeholders::_1, std::placeholders::_2));
	
	conn_->connect(ip, port);
}

Channel::~Channel()
{
	shutdown();
}

void Channel::CallMethod(const ::google::protobuf::MethodDescriptor* method,
                		 ::google::protobuf::RpcController* controller,
						 const ::google::protobuf::Message* request,
                		 ::google::protobuf::Message* response,
						 ::google::protobuf::Closure* done)
{
    /**
 	* FIXME : Ignore the controller here
 	*/ 

	uint64_t id = id_allocator_++;
	MethodInfo method_info(controller, request, response, done);
	id_method_umap_[id] = method_info;
	
	const google::protobuf::ServiceDescriptor *sd = method->service();
    std::string service_name = sd->name();
    std::string method_name = method->name();

	std::string request_str = request->SerializeAsString();

	Message msg;
	msg.set_tp(type::request);
	msg.set_id(id);
	msg.mutable_request()->set_service_name(service_name);
	msg.mutable_request()->set_service_method(method_name);
	msg.mutable_request()->set_arguments(request_str);

	std::string rpc_request_str = msg.SerializeAsString();
	uint32_t len = rpc_request_str.length();
	len = htonl(len);

	conn_->send(std::string((char*)&len, sizeof(uint32_t)) + rpc_request_str);
}

void Channel::shutdown()
{
	is_connected_ = false;
	conn_->shutdown();
}
	
bool Channel::is_connected() const
{
	return is_connected_;
}

lcy::asio::IOContext& Channel::context()
{
	return conn_->context();
}

void Channel::onConnectOp(lcy::asio::errcode_type ec)
{
	if ( !ec ) {
		is_connected_ = true;
		conn_->start_recv();
	}
}

void Channel::onMessageOp(details::Connection& conn, const std::string& msg)
{
	Message rpc_msg;
	if ( !rpc_msg.ParseFromString(msg) ) {
		return;
	}

	if ( rpc_msg.tp() != type::response ) {
		return;
	}

	MethodInfo method_info = id_method_umap_[rpc_msg.id()];
	id_method_umap_.erase(rpc_msg.id());

	/**
 	*	FIXME : Here, ignore the error code and assume the response is correct.
 	*/

	//if ( !rpc_msg.mutable_response()->errcode() ) {
	//	return;
	//}

	method_info.response_->ParseFromString(rpc_msg.mutable_response()->arguments());
	method_info.done_->Run();
}

}	// namespace rpc
}	// namespace lcy
