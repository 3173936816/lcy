#include <signal.h>

#include "lcy/asio/signal_set.hpp"

int main()
{
	lcy::asio::io_context ioc;
	lcy::asio::signal_set signal_set(ioc, { SIGUSR1 });
	bool fired = false;

	signal_set.async_wait([&fired](lcy::system::error_code ec, int signum){
		if ( !ec && signum == SIGUSR1 ) {
			fired = true;
		}
	});

	::raise(SIGUSR1);
	ioc.run();

	return fired ? 0 : 1;
}
