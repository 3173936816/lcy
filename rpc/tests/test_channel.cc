#include "../src/server.h"
#include "../src/channel.h"
#include "../src/callback.h"
#include "test_rpc.pb.h"
#include "lcy/asio/asio.hpp"

#include <iostream>

int main() {
	lcy::asio::IOContext ioc;
	lcy::rpc::Channel channel(ioc, "192.168.21.146", 9950);

	lcy::asio::SteadyTimer timer(ioc, 2000);
	timer.async_wait([&ioc, &channel](lcy::asio::errcode_type, time_t){

		if ( channel.is_connected() ) {
			LoginService_Stub stub(&channel);
			
			LoginRequest* request = new LoginRequest();
			request->set_name("lcy");
			request->set_pwd("123456");

			LoginResponse* response = new LoginResponse();
			lcy::rpc::LambdaClosure* closure = 
					lcy::rpc::NewLambdaClosure([&ioc, request, response](){
				std::cout << response->retcode() << std::endl;

				delete request;
				delete response;

				ioc.quit();
			}, true);

			stub.Login(nullptr, request, response, closure);	
		} else {
			std::cout << "connect error" << std::endl;
		}
	});
	
	ioc.loop_wait();

	return 0;
}
