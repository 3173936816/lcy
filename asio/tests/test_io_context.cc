#include "../asio.hpp"

#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>

using namespace lcy;

void getCurrentTime(const std::string& str) {
    auto now = std::chrono::system_clock::now();
    
    auto time_t_now = std::chrono::system_clock::to_time_t(now);

    std::tm local_time = *std::localtime(&time_t_now);
    
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::cout << str
			  << " 当前时间: " 
              << std::setfill('0') 
              << std::setw(2) << local_time.tm_hour << ":"
              << std::setw(2) << local_time.tm_min << ":"
              << std::setw(2) << local_time.tm_sec << "."
              << std::setw(3) << milliseconds.count() 
              << std::endl;
}

void test_timer(asio::IOContext& ioc)
{
	asio::details::TimerService& timer_service = 
		asio::use_service<asio::details::TimerService>(ioc);

	getCurrentTime("timer start");
	int64_t timer_id = -1;
	timer_service.registerTimer(timer_id, [](int){
		getCurrentTime("timer execute");
	}, {3, 0} );

	int64_t timer_id2 = -1;
	timer_service.registerTimer(timer_id2, [](int){
		getCurrentTime("timer2 execute");
	}, {5, 0} );

	int64_t timer_id3 = -1;
	timer_service.registerTimer(timer_id3, [](int){
		getCurrentTime("timer3 execute");
	}, {10, 0} );

	int64_t timer_id4 = -1;
	timer_service.registerTimer(timer_id4, [&ioc](int){
		ioc.quit();
	}, {12, 0});

	ioc.loop_wait();
}

void producer(asio::IOContext& ioc)
{
	for ( int i = 0; i < 10; ++i ) {
		asio::post(ioc, [i](){ 
			std::cout << "task " << i << " execute" << std::endl; 
			sleep(0.5);
		});
	}

	std::vector<std::function<void()> > callbacks;
	for ( int i = 10; i < 20; ++i ) {
		callbacks.push_back([i](){
			std::cout << "task " << i << " execute" << std::endl; 
			sleep(0.5);
		});
	}
	
	asio::batch(ioc, callbacks.begin(), callbacks.end());
}

void test_bridge(asio::IOContext& ioc)
{
	
	asio::details::TimerService& timer_service = 
		asio::use_service<asio::details::TimerService>(ioc);
	int64_t timer_id = -1;
	timer_service.registerTimer(timer_id, [&ioc](int){
		ioc.quit();
	}, {5, 0});

	std::thread th(producer, std::ref(ioc));

	asio::post(ioc, [](){
		std::cout << "current thread" << std::endl;
	});
	
	ioc.loop_wait();
	
	th.join();
}

int main() {
	asio::IOContext ioc;

	test_timer(ioc);
	// test_bridge(ioc);

	return 0;
}
