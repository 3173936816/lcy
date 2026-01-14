#ifndef __LCY_RPC_SERVER_H__
#define __LCY_RPC_SERVER_H__

#include <string>
#include <unordered_map>

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>

#include "lcy/asio/asio.hpp"
#include "lcy/rpc/src/rpc.pb.h"
#include "lcy/rpc/src/details/server.h"
#include "lcy/rpc/src/details/connection.h"

namespace lcy {
namespace rpc {

class Server {
public:
	Server(lcy::asio::IOContext& ioc,
		   uint32_t threadCount);
	~Server();

	void registerService(::google::protobuf::Service* service);
	void start(const std::string& ip, uint16_t port);
	void stop();

private:
	void onConnectOp(details::Connection& conn);
	void onConnectionMessage(details::Connection& conn, const std::string& data);

	void send_error_response(details::Connection& conn, uint64_t id, lcy::rpc::err errcode);

private:
	class ServiceInfo;
	typedef std::unordered_map<
				std::string,
				ServiceInfo*
			> name_service_info_umap_type;

	details::Server server_;
	name_service_info_umap_type name_service_info_umap_;
};

}	// namespace rpc
}	// namespace lcy

#endif // __LCY_RPC_SERVER_H__
