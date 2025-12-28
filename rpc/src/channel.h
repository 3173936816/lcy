#ifndef __LCY_RPC_CHANNEL_H__
#define __LCY_RPC_CHANNEL_H__

#include <string>
#include <unordered_map>
#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

#include "lcy/asio/asio.hpp"
#include "lcy/rpc/src/details/client_connection.h"

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
	void onConnected(lcy::asio::errcode_type ec);
	void onMessage(lcy::asio::errcode_type ec, 
				   size_t nbytes,
				   lcy::asio::DynamicBuffer& buf);
	void onSendCompletedOp(lcy::asio::errcode_type ec, size_t nbytes);

	enum {
		heart_beat_check_interval = 15000,
		heart_beat_wait_interval = 5000,
	};

private:
	struct MethodInfo;
	typedef time_t time_type;
	typedef uint64_t id_type;
	typedef std::unordered_map<
				message_id_type,
				MethodInfo*
			> id_method_type;

	bool can_be_used_;
	id_type id_allocator_;
	id_method_type id_methods_;
	time_type last_alive_time_;
	lcy::asio::SteadyTimer timer_;
	details::ClientConnection conn_;
};

}	// namespace rpc
}	// namespace lcy

#endif // __LCY_RPC_CHANNEL_H__
