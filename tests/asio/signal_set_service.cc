#include <signal.h>

#include "lcy/asio/detail/scheduler.hpp"
#include "lcy/asio/detail/signal_set_service.hpp"

int main()
{
	lcy::asio::detail::scheduler sch(1);
	lcy::asio::detail::signal_set_service service(sch.get_reactor());
	lcy::asio::detail::signal_set_service::signal_set_impl impl1;
	lcy::asio::detail::signal_set_service::signal_set_impl impl2;

	bool fired1 = false;
	bool fired2 = false;

	service.construct(impl1, { SIGINT });
	service.construct(impl2, { SIGINT });

	service.async_wait(impl1, [&fired1](lcy::system::error_code ec, int signum){
		if ( !ec && signum == SIGINT ) {
			fired1 = true;
		}
	});
	service.async_wait(impl2, [&fired2](lcy::system::error_code ec, int signum){
		if ( !ec && signum == SIGINT ) {
			fired2 = true;
		}
	});

	::raise(SIGINT);
	sch.run();

	if ( !fired1 || !fired2 ) {
		service.destroy(impl1);
		service.destroy(impl2);
		return 1;
	}

	service.destroy(impl1);
	service.destroy(impl2);
	return 0;
}
