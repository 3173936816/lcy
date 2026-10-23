#include <vector>
#include <iostream>

#include "lcy/asio/strand.hpp"

void test_no_strand()
{
	int count = 0;

	lcy::asio::io_context ioc;

	for ( int i = 0; i < 100000; ++i ) {
		lcy::asio::post(ioc, [&count]() mutable {
			int tmp = count;
			tmp = count + 1;
			count = tmp;
		});
	}

	std::vector<std::thread> threads;
	for ( int i = 0; i < 5; ++i ) {
		threads.emplace_back(std::move([&ioc](){
			ioc.run();
		}));
	}

	for ( int i = 0; i < 5; ++i ) {
		threads[i].join();
	}

	std::cout << "no strand count : " << count << std::endl;
}

void test_strand()
{
	int count = 0;

	lcy::asio::io_context ioc;
	lcy::asio::strand strand(ioc);

	for ( int i = 0; i < 100000; ++i ) {
		strand.post([&count]() mutable {
			int tmp = count;
			tmp = count + 1;
			count = tmp;
		});
	}

	std::vector<std::thread> threads;
	for ( int i = 0; i < 5; ++i ) {
		threads.emplace_back(std::move([&ioc](){
			ioc.run();
		}));
	}

	for ( int i = 0; i < 5; ++i ) {
		threads[i].join();
	}

	std::cout << "strand count : " << count << std::endl;
}

void test_strand_wrapper()
{
	lcy::asio::io_context ioc;
	lcy::asio::strand strand(ioc);

	auto func = lcy::asio::strand_wrapper(strand, [](int a, int b, int c){
		std::cout << "strand_wrapper : " << a + b + c << std::endl;
	});

	func(10, 20, 30);

	ioc.run();
}

int main()
{
	test_no_strand();
	test_strand();

	test_strand_wrapper();

	return 0;
}
