#ifndef __LCY_RPC_CHANNEL_H__
#define __LCY_RPC_CHANNEL_H__

#include <string>
#include <functional>
#include <unordered_map>
#include <google/protobuf/service.h>

#include "lcy/asio/asio.hpp"
#include "lcy/rpc/src/details/connection.h"

namespace lcy {
namespace rpc {

class MethodInfo {
public:
	MethodInfo();
  	MethodInfo(::google::protobuf::RpcController* controller,
			   const ::google::protobuf::Message* request,
			   ::google::protobuf::Message* response,
			   ::google::protobuf::Closure* done);
	~MethodInfo();

public:
  	::google::protobuf::RpcController* controller_;
	const ::google::protobuf::Message* request_;
	::google::protobuf::Message* response_;
	::google::protobuf::Closure* done_;
};

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

	void shutdown();
	bool is_connected() const;
	lcy::asio::IOContext& context();

private:
	void onConnectOp(lcy::asio::errcode_type ec);
	void onMessageOp(details::Connection& conn, const std::string& msg);

private:
	typedef std::unordered_map<
				uint64_t,
				MethodInfo
			> id_method_umap_type;

	bool is_connected_;
	uint64_t id_allocator_;
	std::shared_ptr<details::Connection> conn_;
	id_method_umap_type id_method_umap_;
};

}	// namespace rpc
}	// namespace lcy

#endif // __LCY_RPC_CHANNEL_H__
