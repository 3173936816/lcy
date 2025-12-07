#ifndef __LCY_ASIO_SIGNAL_SET_H__
#define __LCY_ASIO_SIGNAL_SET_H__

#include "asio/src/details/channel.h"

#include <functional>
#include <initializer_list>

namespace lcy {
namespace asio {

class IOContext;

class SignalSet {
public:
	typedef int signal_type;
	typedef std::function<void (int, signal_type)> signal_callback_type;

	SignalSet(IOContext& ioc, signal_type signum);
	~SignalSet();

	void async_wait(signal_callback_type signal_cb);
	void cancel();

private:
	SignalSet(const SignalSet&);
	SignalSet& operator=(const SignalSet&);

private:
	signal_type signum_;
	details::Channel channel_;
};

}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_SIGNAL_SET_H__
