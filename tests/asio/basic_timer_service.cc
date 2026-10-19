#include "lcy/asio/detail/steady_timer_service.hpp"
#include "lcy/asio/detail/scheduler.hpp"

int main()
{
	lcy::asio::detail::scheduler sch(1);
	lcy::asio::detail::steady_timer_service service(sch.get_reactor());
	lcy::asio::detail::steady_timer_service::timer_impl impl;
	service.construct(impl);

	bool fired = false;
	service.expires_after(impl, 10);
	service.async_wait(impl, [&fired](lcy::system::error_code ec){
		if ( !ec ) {
			fired = true;
		}
	});

	sch.run();
	if ( !fired ) {
		service.destroy(impl);
		return 1;
	}

	service.destroy(impl);
	return 0;
}
