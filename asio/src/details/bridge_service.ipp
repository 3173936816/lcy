namespace lcy {
namespace asio {
namespace details {

template <typename Iterator>
void BridgeService::batchNoLock(Iterator beg, Iterator end)
{
	for ( ; beg != end; ++beg ) {
		task_list_.emplace_back(*beg);
	}

	uint64_t u = 1;
	if ( ::write(channel_.fd(), &u, sizeof(u)) != sizeof(u) ) {
		if ( errno != EAGAIN ) {
			throw LcyAsioException("eventfd write");
		}
	}
}

template <typename Iterator>
void BridgeService::batch(Iterator beg, Iterator end)
{
	std::lock_guard<std::mutex> locker(mutex_);
	batchNoLock(beg, end);
}

}	// namespace details
}	// namespace asio
}	// namespace lcy
