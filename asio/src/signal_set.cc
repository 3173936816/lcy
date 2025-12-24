#include "lcy/asio/src/signal_set.h"
#include "lcy/asio/src/exception.h"
#include "lcy/asio/src/io_context.hpp"
#include "lcy/asio/src/errinfo.h"

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

	::pthread_sigmask(SIG_BLOCK, &sigset, nullptr);
}

static void signal_unblock(int signum)
{
	sigset_t sigset;
	::sigemptyset(&sigset);

	::sigaddset(&sigset, signum);

	::pthread_sigmask(SIG_UNBLOCK, &sigset, nullptr);
}

///////////////////////////////////////////////////////////////

static void signal_op_wrap(errcode_type ec,
						   int signal_fd,
						   details::ReactorService& reactor,
						   SignalSet::signal_op_type signal_op)
{
	if ( !ec ) {
		reactor.removeReadOperation(signal_fd);

		struct signalfd_siginfo info;
		::read(signal_fd, &info, sizeof(info));

		signal_op(ec, info.ssi_signo);
	} else {
		/*
		*notify:
		*  This error was notified by the reactor ( mybe EOPEXISTS, EOPCANCELED )
		*  We just need to notify the caller.
		*/ 

		signal_op(ec, -1);
	}
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
			signal_op_wrap, std::placeholders::_1, signal_fd_, 
				std::ref(reactor_),  std::move(signal_op)));
	
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
