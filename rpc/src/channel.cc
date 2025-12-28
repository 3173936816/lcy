#include "lcy/rpc/src/channel.h"
#include "lcy/rpc/src/rpc.pb.h"

#include "lcy/rpc/src/closure.h"

namespace lcy {
namespace rpc {

struct MthhodInfo {
	::google::protobuf::RpcController* controller_;
	::google::protobuf::Message* request_;
	::google::protobuf::Message* response_;
	std::shared_ptr<::google::protobuf::Closure> done_;
	std::string serlized_data_;
};

///////////////////////////////////////////////////////////

Channel::Channel(lcy::asio::IOContext& ioc,
				 const std::string& ip,
				 uint16_t port) :
	id_allocator_(0),
	conn_(ioc),
	timer_(ioc, 5)
{
	conn_.async_connect(ip, prot, std::bind(
			&Channel::onConnected, this, std::placeholders::_1));
}

Channel::~Channel()
{
	conn_.shutdown();

	for ( auto kv : id_methods_ ) {
		delete kv.second;
	}
}

void Channel::CallMethod(const ::google::protobuf::MethodDescriptor* method,
    		    		 ::google::protobuf::RpcController* controller,
			    		 const ::google::protobuf::Message* request,
    		    		 ::google::protobuf::Message* response,
			    		 ::google::protobuf::Closure* done)
{
}

void Channel::onConnected(lcy::asio::errcode_type ec)
{
	if ( !ec ) {

	} else {
		startConnectTimer();
	}
}

void Channel::onMessage(lcy::asio::errcode_type ec, 
			   			size_t nbytes,
			   			lcy::asio::DynamicBuffer& buf)
{
}

void Channel::onSendCompletedOp(lcy::asio::errcode_type ec, size_t nbytes)
{
}

void Channel::onTimer(lcy::asio::errcode_type ec, time_t timeout)
{
	
}

void Channel::startTimer();
{
	timer_.async_wait(std::bind(&Channel::onTimer,
			this, std::placeholder::_1, std::placeholder::_2));
}

}	// namespace rpc
}	// namespace lcy
