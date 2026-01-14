#include "../src/server.h"
#include "test_rpc.pb.h"
#include "lcy/asio/asio.hpp"

#include <iostream>

class LoginServiceTest :
	public LoginService
{
public:
	  void Login(::google::protobuf::RpcController* controller,
                 const ::LoginRequest* request,
                 ::LoginResponse* response,
                 ::google::protobuf::Closure* done) override
		{
			std::cout << "Login : " << request->name() 
					  << " " << request->pwd() << std::endl;
			response->set_retcode(RetCode::SUCCESS);
			
			done->Run();
		}
};

int main() {
	lcy::asio::IOContext ioc;
	lcy::asio::SignalSet signal_set(ioc, SIGINT);
	signal_set.async_wait([&ioc](lcy::asio::errcode_type ec, int signum){
		ioc.quit();
	});

	lcy::rpc::Server server(ioc, 3);
	server.registerService(new LoginServiceTest());

	server.start("0.0.0.0", 9950);
	
	ioc.loop_wait();

	return 0;
}
