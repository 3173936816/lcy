#include <list>
#include <unistd.h>

#include "lcy/asio/detail/basic_timer_set.hpp"
#include "lcy/asio/detail/timer_set_queue.hpp"

struct steady_clock
{
	static long long now()
	{
		timespec ts;
		::clock_gettime(CLOCK_MONOTONIC, &ts);

		return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
	}
};

struct system_clock
{
	static long long now()
	{
		timespec ts;
		::clock_gettime(CLOCK_REALTIME, &ts);

		return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
	}
};

int main()
{
	lcy::asio::detail::timer_set_queue tsq;
	lcy::asio::detail::basic_timer_set<steady_clock> steady_timer_set;
	lcy::asio::detail::basic_timer_set<system_clock> system_timer_set;

	tsq.add_timer_set(&steady_timer_set);
	tsq.add_timer_set(&system_timer_set);

	auto internal_timer1 = tsq.alloc();
	auto internal_timer2 = tsq.alloc();
	auto internal_timer3 = tsq.alloc();
	auto internal_timer4 = tsq.alloc();

	bool triggered = false;
	steady_timer_set.insert_internal_timer(internal_timer1,
		steady_clock::now() + 10,
		lcy::asio::detail::operation([&triggered](lcy::system::error_code ec, int){
			(void)ec;
			triggered = true;
		}));

	if ( !steady_timer_set.has_internal_timer(internal_timer1) ) {
		return 1;
	}

	::usleep(20 * 1000);

	std::list<lcy::asio::detail::operation> op_list;
	tsq.get_ready_internal_timer_ops(op_list);
	if ( op_list.empty() ) {
		return 2;
	}

	for ( auto& op : op_list ) {
		op();
	}

	if ( !triggered ) {
		return 3;
	}

	if ( steady_timer_set.has_internal_timer(internal_timer1) ) {
		return 4;
	}

	tsq.free(internal_timer1);
	tsq.free(internal_timer2);
	tsq.free(internal_timer3);
	tsq.free(internal_timer4);

	return 0;
}
