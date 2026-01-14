#include "lcy/rpc/src/server.h"
#include "lcy/rpc/src/rpc.pb.h"
#include "lcy/rpc/src/callback.h"

#include <iostream>

namespace lcy {
namespace rpc {

class Server::ServiceInfo {
public:
	ServiceInfo(::google::protobuf::Service* service);
	~ServiceInfo();

	::google::protobuf::Service* getService();
	const google::protobuf::MethodDescriptor* getMethod(const std::string& name) const;

private:
	typedef std::unordered_map<
				std::string,
				const google::protobuf::MethodDescriptor*
			> name_method_umap_type;

	google::protobuf::Service* service_;
	name_method_umap_type name_methods_umap_;
};

/////////////////////////////////////////////////////

Server::ServiceInfo::ServiceInfo(::google::protobuf::Service* service) :
	service_(service)
{
	const ::google::protobuf::ServiceDescriptor* serviceDesc = service->GetDescriptor();
	int method_count = serviceDesc->method_count();

	for ( int i = 0; i < method_count; ++i ) {
		const ::google::protobuf::MethodDescriptor* methodDesc = serviceDesc->method(i);
		std::string method_name = methodDesc->name();

		name_methods_umap_[method_name] = methodDesc;
	}
}

::google::protobuf::Service* Server::ServiceInfo::getService()
{
	return service_;
}

const google::protobuf::MethodDescriptor* Server::ServiceInfo::getMethod(const std::string& name) const
{
	auto iter = name_methods_umap_.find(name);
	if ( iter == name_methods_umap_.end() ) {
		return nullptr;
	}
	
	return iter->second;
}

Server::ServiceInfo::~ServiceInfo()
{
	delete service_;
}

/////////////////////////////////////////////////////

Server::Server(lcy::asio::IOContext& ioc,
	   		   uint32_t threadCount) :
	server_(ioc, threadCount)
{
	server_.setConnectOp(std::bind(&Server::onConnectOp,
			this, std::placeholders::_1));
}

Server::~Server()
{
	stop();	

	for ( auto& kv : name_service_info_umap_ ) {
		delete kv.second;
	}
}

void Server::stop()
{
	server_.stop();
}

void Server::registerService(::google::protobuf::Service* service)
{
	const ::google::protobuf::ServiceDescriptor* serviceDesc = service->GetDescriptor();
	std::string service_name = serviceDesc->name();

	name_service_info_umap_[service_name] = new ServiceInfo(service);
}

void Server::start(const std::string& ip, uint16_t port)
{
	lcy::asio::ip::Endpoint endpoint(ip, port);
	server_.start(endpoint);
}

void Server::onConnectOp(details::Connection& conn)
{
	conn.setMessageOp(std::bind(&Server::onConnectionMessage,
			this, std::placeholders::_1, std::placeholders::_2));
}

void Server::onConnectionMessage(details::Connection& conn, const std::string& data)
{
	lcy::rpc::Message msg;
	if ( msg.ParseFromString(data) ) {
		if ( msg.tp() != lcy::rpc::request ) {	// The server only accepts requests
			send_error_response(conn, msg.id(), lcy::rpc::err::NOT_SUPPORT);
			return;
		}

		uint64_t id = msg.id();
		std::string service_name = msg.request().service_name();
		std::string service_method = msg.request().service_method();
		std::string arguments = msg.request().arguments();

		auto iter = name_service_info_umap_.find(service_name);
		if ( iter == name_service_info_umap_.end() ) {
			send_error_response(conn, id, lcy::rpc::err::NO_SERVICE);
			return;
		}

		ServiceInfo* service_info = iter->second;
		::google::protobuf::Service* service = service_info->getService();
		const ::google::protobuf::MethodDescriptor* mdesc = service_info->getMethod(service_method);
		if ( !mdesc ) {
			send_error_response(conn, id, lcy::rpc::err::NO_METHOD);
			return;
		}

		::google::protobuf::Message *request = service->GetRequestPrototype(mdesc).New();
		if ( !request->ParseFromString(arguments) ) {
			send_error_response(conn, id, lcy::rpc::err::INVALID_REQUEST);
			return;
		}
		::google::protobuf::Message *response = service->GetResponsePrototype(mdesc).New();
	    ::google::protobuf::Closure* done = NewLambdaClosure([&conn, id, response](){
			std::string response_str;
			response->SerializeToString(&response_str);

			Message rpc_response;
			rpc_response.set_id(id);
			rpc_response.set_tp(type::response);
			rpc_response.mutable_response()->set_errcode(err::SUCCESS);
			rpc_response.mutable_response()->set_arguments(response_str);
			
			std::string rpc_response_str;
			rpc_response.SerializeToString(&rpc_response_str);

			uint32_t len = rpc_response_str.length();
			len = htonl(len);
			std::string rpc_lv_response(std::string((char*)&len, sizeof(uint32_t)) + rpc_response_str);
			conn.send(std::move(rpc_lv_response));
		}, true);
		
		/**
 		 *	FIXME : Internal calls may fail, so it's best to use control parameters.
 		 *	This is just a very simple demonstration.
 		 */ 	
		service->CallMethod(mdesc, nullptr, request, response, done);

		delete request;
		delete response;
	
	} else {
		 /**
 		 *	FIXME : There is an error in the RPC system message here;
 		 *	the ID is missing. Consider possible solutions.
 		 */ 
		// send_error_response(conn, id ?, lcy::rpc::err::INVALID_REQUEST);
	}
}

void Server::send_error_response(details::Connection& conn, uint64_t id, lcy::rpc::err errcode)
{
	Message rpc_error_response;
	rpc_error_response.set_id(id);
	rpc_error_response.set_tp(type::response);
	rpc_error_response.mutable_response()->set_errcode(errcode);

	std::string rpc_error_response_str;
	rpc_error_response.SerializeToString(&rpc_error_response_str);

	uint32_t len = rpc_error_response_str.length();
	len = htonl(len);
	std::string rpc_error_lv_response(std::string((char*)&len, sizeof(uint32_t)) + rpc_error_response_str);
	conn.send(std::move(rpc_error_lv_response));
}

}	// namespace rpc
}	// namespace lcy
