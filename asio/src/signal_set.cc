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
	if ( fd != -1 && ::close(fd) ) {
		throw LcyAsioException("signalfd close");
	}
}

static void update_signalfd(int fd, int signum)
{
	sigset_t sigset;
	::sigemptyset(&sigset);

	::sigaddset(&sigset, signum);

	int signal_fd = ::signalfd(fd, &sigset, SFD_CLOEXEC | SFD_NONBLOCK);
	if ( signal_fd == -1 ) {
		throw LcyAsioException("signalfd update");
	}
}

static void signal_block(int signum)
{
	sigset_t sigset;
	::sigemptyset(&sigset);

	::sigaddset(&sigset, signum);

	if ( ::sigprocmask(SIG_BLOCK, &sigset, nullptr) ) {
		throw LcyAsioException("signal block");
	}
}

static void signal_unblock(int signum)
{
	sigset_t sigset;
	::sigemptyset(&sigset);

	::sigaddset(&sigset, signum);

	if ( ::sigprocmask(SIG_UNBLOCK, &sigset, nullptr) ) {
		throw LcyAsioException("signal unblock");
	}
}

///////////////////////////////////////////////////////////////

static void signal_cb_wrap(details::Channel& channel, 
						   SignalSet::signal_callback_type signal_cb)
{
	struct signalfd_siginfo info;
	if ( ::read(channel.fd(), &info, sizeof(info)) != sizeof(info) ) {
		signal_cb(err::ESYSTEM, info.ssi_signo);
		return;
	}

	signal_cb(err::SUCCESS, info.ssi_signo);
}

///////////////////////////////////////////////////////////////

SignalSet::SignalSet(IOContext& ioc, signal_type signum) :
	signum_(signum),
	channel_(create_signalfd(), use_service<details::ReactorService>(ioc))
{
}

SignalSet::~SignalSet()
{
	cancel();
	destroy_signalfd(channel_.fd());
}

void SignalSet::async_wait(signal_callback_type signal_cb)
{
	if ( channel_.registerReadCallback(std::bind(
			signal_cb_wrap, std::ref(channel_), signal_cb)) ) {		// signal_cb can not be moved
		signal_cb(err::EREGEVENT, -1);								// this need to be used
		return;
	}

	update_signalfd(channel_.fd(), signum_);
	signal_block(signum_);
}

void SignalSet::cancel()
{
	channel_.cancelReading();		// FIXME : check return value
	signal_unblock(signum_);
}

}	// namespace asio
}	// namespace lcy
