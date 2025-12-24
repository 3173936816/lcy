#include "lcy/asio/src/io_context.hpp"

namespace lcy {
namespace asio {

IOContext::IOContext() :
	thread_id_(std::this_thread::get_id())
{
}

IOContext::~IOContext()
{
	uintptr_t reactor_id = (uintptr_t)&details::ServiceId<details::ReactorService>::id;

 /*
 * Notify:
 *	First unregister the other services, 
 *	and finally unregister the reactor to ensure the correctness of the state.
 */

	for ( auto& id_service : id_service_umap_ ) {
		if ( id_service.first != reactor_id ) {
			delete id_service.second;
		}
	}
	
	delete id_service_umap_[reactor_id];
}

void IOContext::quit()
{
	use_service<details::ReactorService>(*this).quit();
}

errcode_type IOContext::loop_wait()
{
	return use_service<details::ReactorService>(*this).loop_wait();
}

void post(IOContext& ioc, task_op_type task_op)
{
	if ( ioc.thread_id_ == std::this_thread::get_id() ) {
		task_op();
		return;
	}
	
	details::BridgeService& bridge_service = use_service<details::BridgeService>(ioc);
	bridge_service.push(std::move(task_op));
}

}	// namespace asio
}	// namespace lcy
