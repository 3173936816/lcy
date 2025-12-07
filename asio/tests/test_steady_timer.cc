#include "../asio.hpp"

#include <iostream>
#include <functional>
#include <chrono>
#include <ctime>
#include <iomanip>

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

void timer_cb(lcy::asio::SteadyTimer& timer, int errcode, time_t timeout)
{
	std::cout << "timeout : " << timeout << std::endl;

	getCurrentTime("execute");
}

int main() {
	lcy::asio::IOContext ioc;
	lcy::asio::SteadyTimer timer(ioc, 1000);

	getCurrentTime("start");
	timer.async_wait(std::bind(timer_cb, std::ref(timer), std::placeholders::_1, std::placeholders::_2));
	//timer.async_wait(std::bind(timer_cb, std::ref(timer), std::placeholders::_1, std::placeholders::_2));

	ioc.loop_wait();
	
	return 0;
}
