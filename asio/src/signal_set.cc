#include "asio/src/signal_set.h"
#include "asio/src/exception.h"
#include "asio/src/io_context.hpp"
#include "asio/src/errinfo.h"

#include <signal.h>
#include <unistd.h>
#include <sys/signalfd.h>

namespace lcy {
namespace asio {

static int create_signalfd()
{
	sigset_t sigset;
	::sigemptyset(&sigset);

	int signal_fd = ::signalfd(-1, &sigset, SFD_CLOEXEC | SFD_NONBLOCK);
	if ( signal_fd == -1 ) {
		throw LcyAsioException("signalfd create");
	}

	return signal_fd;
}

static void destroy_signalfd(int fd)
{
	if ( fd != -1 ) {
		::close(fd);
	}
}

static void update_signalfd(int fd, int signum)
{
	sigset_t sigset;
	::sigemptyset(&sigset);

	::sigaddset(&sigset, signum);

	::signalfd(fd, &sigset, SFD_CLOEXEC | SFD_NONBLOCK);
}

static void signal_block(int signum)
{
	sigset_t sigset;
	::sigemptyset(&sigset);

	::sigaddset(&sigset, signum);

	::sigprocmask(SIG_BLOCK, &sigset, nullptr);
}

static void signal_unblock(int signum)
{
	sigset_t sigset;
	::sigemptyset(&sigset);

	::sigaddset(&sigset, signum);

	::sigprocmask(SIG_UNBLOCK, &sigset, nullptr);
}

///////////////////////////////////////////////////////////////

static void signal_op_wrap(int signal_fd,
						   errcode_type ec,
						   SignalSet::signal_op_type signal_op)
{
	struct signalfd_siginfo info;
	::read(signal_fd, &info, sizeof(info));

	signal_op(ec, info.ssi_signo);
}

///////////////////////////////////////////////////////////////

SignalSet::SignalSet(IOContext& ioc, signal_type signum) :
	ioc_(ioc),
	signum_(signum),
	signal_fd_(create_signalfd()),
	reactor_(use_service<details::ReactorService>(ioc))
{
}

SignalSet::~SignalSet()
{
	reactor_.cancelAllOperations(signal_fd_);
	destroy_signalfd(signal_fd_);
	signal_unblock(signum_);
}

void SignalSet::async_wait(signal_op_type signal_op)
{	
	reactor_.registerReadOperation(signal_fd_, std::bind(
			signal_op_wrap, signal_fd_, std::placeholders::_1, std::move(signal_op)));
	
	update_signalfd(signal_fd_, signum_);
	signal_block(signum_);
}

void SignalSet::cancel()
{
	reactor_.cancelReadOperation(signal_fd_);
	signal_unblock(signum_);
}

IOContext& SignalSet::context()
{
	return ioc_;
}

}	// namespace asio
}	// namespace lcy
