#ifndef __LCY_ASIO_SIGNAL_SET_H__
#define __LCY_ASIO_SIGNAL_SET_H__

#include <functional>

#include "lcy/asio/src/errinfo.h"

namespace lcy {
namespace asio {
namespace details {
	class ReactorService;
}	// namespace details

class IOContext;

class SignalSet {
public:
	typedef int signal_type;
	typedef std::function<void (errcode_type, signal_type)> signal_op_type;

	SignalSet(IOContext& ioc, signal_type signum);
	~SignalSet();

	void async_wait(signal_op_type signal_op);
	void cancel();

	IOContext& context();

private:
	SignalSet(const SignalSet&);
	SignalSet& operator=(const SignalSet&);

private:
	typedef int signalfd_type;

	IOContext& ioc_;
	signal_type signum_;
	signalfd_type signal_fd_;
	details::ReactorService& reactor_;
};

}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_SIGNAL_SET_H__
