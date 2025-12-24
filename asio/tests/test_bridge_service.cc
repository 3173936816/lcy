#include "../asio.hpp"

#include <iostream>
#include <thread>

using namespace lcy;

void producer(asio::details::BridgeService& bridge)
{
	for ( int i = 0; i < 10; ++i ) {
		bridge.push([i](){ 
			std::cout << "task " << i << " execute" << std::endl; 
			sleep(1);
		});
	}

	std::vector<std::function<void()> > callbacks;
	for ( int i = 10; i < 20; ++i ) {
		callbacks.push_back([i](){
			std::cout << "task " << i << " execute" << std::endl; 
			sleep(1);
		});
	}
	
	bridge.batch(callbacks.begin(), callbacks.end());
}

int main() {
	asio::details::ReactorService reactor;
	asio::details::BridgeService bridge(reactor);

	std::thread th(producer, std::ref(bridge));
	
	reactor.loop_wait();
	
	th.join();

	return 0;
}
