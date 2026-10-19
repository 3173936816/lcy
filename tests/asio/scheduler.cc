#include "lcy/asio/detail/scheduler.hpp"

int main()
{
	lcy::asio::detail::scheduler sch(1);
	bool executed = false;
	sch.operation_increase();
	sch.submit(lcy::asio::detail::operation([&executed](lcy::system::error_code ec, int){
		(void)ec;
		executed = true;
	}));

	std::size_t count = sch.run();
	if ( count != 1 || !executed ) {
		return 1;
	}

	sch.stop();
	return 0;
}