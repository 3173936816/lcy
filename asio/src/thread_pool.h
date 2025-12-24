#ifndef __LCY_ASIO_THREAD_POOL_H__
#define __LCY_ASIO_THREAD_POOL_H__

#include <vector>
#include <atomic>
#include <stddef.h>

namespace lcy {
namespace asio {

class IOContext;

class ThreadPool {
public:
	ThreadPool(size_t thread_num);
	~ThreadPool();

	void start();
	void stop();

	IOContext& nextContext();

private:
	ThreadPool(const ThreadPool&);
	ThreadPool& operator=(const ThreadPool);	

private:
	class Thread;
	typedef std::atomic<size_t> atomic_size_type;
	typedef std::vector<Thread*> thread_array_type;
	
	bool is_start_;
	atomic_size_type next_;
	thread_array_type threads_;
};

}	// namespace asio
}	// namespace lcy

#endif	// __LCY_ASIO_THREAD_POOL_H__
