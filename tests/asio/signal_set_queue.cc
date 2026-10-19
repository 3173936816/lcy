#include <list>
#include <signal.h>
#include <initializer_list>

#include "lcy/asio/detail/signal_set_queue.hpp"

static uint64_t make_mask(std::initializer_list<int> signums)
{
	uint64_t mask = 0;
	for ( int signum : signums ) {
		mask |= static_cast<uint64_t>(1) << (signum - 1);
	}

	return mask;
}

int main()
{
	lcy::asio::detail::signal_set_queue ssq;
	auto iss1 = ssq.alloc();
	auto iss2 = ssq.alloc();

	bool triggered = false;
	ssq.add_internal_signal_set(iss1, make_mask({ SIGINT, SIGHUP }),
		lcy::asio::detail::operation([&triggered](lcy::system::error_code ec, int signum){
			(void)signum;
			if ( !ec ) {
				triggered = true;
			}
		}));
	ssq.add_internal_signal_set(iss2, make_mask({ SIGTERM }),
		lcy::asio::detail::operation([](lcy::system::error_code ec, int){
			(void)ec;
		}));

	if ( !ssq.has_internal_signal_set(iss1) ) {
		return 1;
	}

	std::list<lcy::asio::detail::operation> op_list;
	ssq.get_ready_internal_signal_set_ops(SIGINT, op_list);
	if ( op_list.empty() ) {
		return 2;
	}

	for ( auto& op : op_list ) {
		op();
	}

	if ( !triggered || ssq.has_internal_signal_set(iss1) ) {
		return 3;
	}

	ssq.free(iss1);
	ssq.free(iss2);
	return 0;
}
