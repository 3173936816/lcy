#include "asio/src/details/channel.h"
#include "asio/src/details/reactor_service.h"

namespace lcy {
namespace asio {
namespace details {

Channel::Channel(ReactorService& reactor) :
	fd_(-1),
	reactor_(&reactor)
{
}

Channel::Channel(file_descriptor_type fd,
				 ReactorService& reactor) :
	fd_(fd),
	reactor_(&reactor)
{
}

Channel::~Channel()
{
}

Channel::file_descriptor_type Channel::fd() const
{
	return fd_;
}

void Channel::setFd(file_descriptor_type fd)
{
	fd_ = fd;
}

int Channel::cancelAll()
{
	async_callback_type read_tmp_cb;
	async_callback_type write_tmp_cb;

	std::swap(read_tmp_cb, read_cb_);
	std::swap(write_tmp_cb, write_cb_);

	int ret = 0;
	ret = reactor_->removeChannel(*this);
	if ( ret ) {
		std::swap(read_tmp_cb, read_cb_);
		std::swap(write_tmp_cb, write_cb_);
	}

	return ret;
}

int Channel::cancelReading()
{
	async_callback_type tmp_cb;

	std::swap(tmp_cb, read_cb_);

	int ret = 0;
	if ( write_cb_ )
		ret = reactor_->modifyChannel(*this);
	else
		ret = reactor_->removeChannel(*this);
	
	if ( ret ) {
		std::swap(tmp_cb, read_cb_);
	}
	
	return ret;
}

int Channel::cancelWriting()
{
	async_callback_type tmp_cb;

	std::swap(tmp_cb, write_cb_);

	int ret = 0;
	if ( read_cb_ )
		ret = reactor_->modifyChannel(*this);
	else
		ret = reactor_->removeChannel(*this);

	if ( ret ) {
		std::swap(tmp_cb, write_cb_);
	}
	
	return ret;
}

Channel::async_callback_type Channel::readCallback() const
{
	return read_cb_;
}

Channel::async_callback_type Channel::writeCallback() const
{
	return write_cb_;
}

int Channel::registerReadCallback(async_callback_type read_cb)
{
	bool flag = (read_cb_ != nullptr);

	std::swap(read_cb_, read_cb);

	int ret = 0;
	if ( flag || write_cb_ )
		ret = reactor_->modifyChannel(*this);
	else
		ret = reactor_->registerChannel(*this);

	if ( ret ) {
		std::swap(read_cb_, read_cb);
	}
	
	return ret;
}

int Channel::registerWriteCallback(async_callback_type write_cb)
{
	bool flag = (write_cb_ != nullptr);

	std::swap(write_cb_, write_cb);

	int ret = 0;
	if ( flag || read_cb_ )
		ret = reactor_->modifyChannel(*this);
	else
		ret = reactor_->registerChannel(*this);
	
	if ( ret ) {
		std::swap(write_cb_, write_cb);
	}

	return ret;
}

ReactorService& Channel::reactor()
{
	return *reactor_;
}

}	// namespace details
}	// namespace asio
}	// namespace lcy
