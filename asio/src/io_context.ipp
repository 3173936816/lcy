namespace lcy {
namespace asio {

template <typename service>
service& use_service(IOContext& ioc)
{
	throw LcyAsioException("use_service no such service");
}

template <>
inline details::ReactorService& use_service<details::ReactorService>(IOContext& ioc)
{
	std::lock_guard<std::mutex> locker(ioc.mutex_);

	uintptr_t id = (uintptr_t)&details::ServiceId<details::ReactorService>::id;
	if ( ioc.id_service_umap_.find(id) == ioc.id_service_umap_.end() ) {
		ioc.id_service_umap_[id] = new details::ReactorService();
	}

	return static_cast<
		   		details::ReactorService&
		   >(*(ioc.id_service_umap_[id]));
}

template <>
inline details::TimerService& use_service<details::TimerService>(IOContext& ioc)
{
	details::ReactorService& reactor = use_service<details::ReactorService>(ioc);

	std::lock_guard<std::mutex> locker(ioc.mutex_);

	uintptr_t id = (uintptr_t)&details::ServiceId<details::TimerService>::id;
	if ( ioc.id_service_umap_.find(id) == ioc.id_service_umap_.end() ) {
		ioc.id_service_umap_[id] = new details::TimerService(reactor);
	}

	return static_cast<
		   		details::TimerService&
		   >(*(ioc.id_service_umap_[id]));
}

template <>
inline details::BridgeService& use_service<details::BridgeService>(IOContext& ioc)
{
	details::ReactorService& reactor = use_service<details::ReactorService>(ioc);

	std::lock_guard<std::mutex> locker(ioc.mutex_);

	uintptr_t id = (uintptr_t)&details::ServiceId<details::BridgeService>::id;
	if ( ioc.id_service_umap_.find(id) == ioc.id_service_umap_.end() ) {
		ioc.id_service_umap_[id] = new details::BridgeService(reactor);
	}

	return static_cast<
		   		details::BridgeService&
		   >(*(ioc.id_service_umap_[id]));
}

template <typename Iterator>
inline void batch(IOContext& ioc, Iterator beg, Iterator end)
{
	if ( ioc.thread_id_ == std::this_thread::get_id() ) {
		for ( ; beg != end; ++beg ) {
			(*beg)();
		}
		return;
	}

	details::BridgeService& bridge_service = use_service<details::BridgeService>(ioc);
	bridge_service.batch(beg, end);
}

}	// namespace asio
}	// namespace lcy
