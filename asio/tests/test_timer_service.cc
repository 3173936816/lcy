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


int main() {
	asio::details::ReactorService reactor;
	asio::details::TimerService timer_service(reactor);

	int64_t timer_id2 = -1;

	getCurrentTime("timer start");
	int64_t timer_id = -1;
	timer_service.registerTimer(timer_id, [&timer_id2, &timer_service](int errcode){
		getCurrentTime("timer execute");

		timer_service.cancelTimer(timer_id2);
	}, {3, 0} );

	timer_service.registerTimer(timer_id2, [](int errcode){
		if ( !errcode ) {
			getCurrentTime("timer2 execute");
		} else {
			std::cout << "timer2 " << lcy::asio::errinfo(errcode) << std::endl;
		}
	}, {5, 0} );

	int64_t timer_id3 = -1;
	timer_service.registerTimer(timer_id, [](int errcode){
		if ( !errcode ) {
			getCurrentTime("timer3 execute");
		} else {
			std::cout << "timer3 " << lcy::asio::errinfo(errcode) << std::endl;
		}
	}, {10, 0} );

	int64_t timer_id4 = -1;
	timer_service.registerTimer(timer_id4, [&reactor](int errcode){
		reactor.quit();
	}, {7, 0});

	reactor.loop_wait();

	return 0;
}
