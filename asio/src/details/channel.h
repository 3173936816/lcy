#ifndef __LCY_ASIO_DETAILS_CHANNEL_H__
#define __LCY_ASIO_DETAILS_CHANNEL_H__

#include <functional>

namespace lcy {
namespace asio {
namespace details {

class ReactorService;

class Channel {
public:
	typedef int file_descriptor_type;
	typedef std::function<void ()> async_callback_type;

	Channel(ReactorService& reactor);
	Channel(file_descriptor_type fd,
			ReactorService& reactor);
	~Channel();

	file_descriptor_type fd() const;
	void setFd(file_descriptor_type fd);

	int cancelAll();
	int cancelReading();
	int cancelWriting();
	async_callback_type readCallback() const;
	async_callback_type writeCallback() const;
	int registerReadCallback(async_callback_type read_cb);
	int registerWriteCallback(async_callback_type write_cb);
	
	ReactorService& reactor();

private:
	Channel(const Channel&);
	Channel& operator=(const Channel&);

private:
	file_descriptor_type fd_;
	ReactorService* reactor_;
	async_callback_type read_cb_;
	async_callback_type write_cb_;
};

}	// namespace details
}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_DETAILS_CHANNEL_H__
