#ifndef __LCY_RPC_CHANNEL_H__
#define __LCY_RPC_CHANNEL_H__

#include <atomic>
#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

#include "lcy/asio/asio.hpp"

namespace lcy {
namespace rpc {

class Channel :
	public ::google::protobuf::RpcChannel
{
public:
	Channel(lcy::asio::IOContext& ioc,
			const std::string& ip,
			uint16_t port);
	~Channel();

	void CallMethod(const ::google::protobuf::MethodDescriptor* method,
                    ::google::protobuf::RpcController* controller,
					const ::google::protobuf::Message* request,
                    ::google::protobuf::Message* response,
					::google::protobuf::Closure* done) override;

private:
	typedef uint64_t rpc_message_id_type;

	rpc_message_id_type id_allocator_;
	lcy::asio::ip::TCP::Socket rpc_socket_;
};

}	// namespace rpc
}	// namespace lcy

#endif // __LCY_RPC_CHANNEL_H__
