#include "lcy/asio/src/thread_pool.h"
#include "lcy/asio/src/io_context.hpp"
#include "lcy/asio/src/exception.h"

#include <thread>
#include <pthread.h>
#include <semaphore.h>

namespace lcy {
namespace asio {

class ThreadPool::Thread {
public:
	Thread();
	~Thread();

	void start();
	void stop();
	
	IOContext& context();

private:
	sem_t sem_;
	IOContext* ioc_;
	std::thread th_;
};

/////////////////////////////////////////

ThreadPool::Thread::Thread() :
	ioc_(nullptr)
{
	::sem_init(&sem_, 0, 0);
}

ThreadPool::Thread::~Thread()
{
	::sem_destroy(&sem_);
}

void ThreadPool::Thread::start()
{
	th_ = std::thread([this](){ 
		IOContext ioc;
		ioc_ = &ioc;
		::sem_post(&sem_);

		ioc.loop_wait(); 
		ioc_ = nullptr;
	});
	
	::sem_wait(&sem_);
}

void ThreadPool::Thread::stop()
{
	post(*ioc_, [this](){ ioc_->quit(); });
	th_.join();
}

IOContext& ThreadPool::Thread::context()
{
	return *ioc_;
}

/////////////////////////////////////////////////

ThreadPool::ThreadPool(size_t thread_num) :
	is_start_(false),
	next_(0)
{
	if ( thread_num < 0 ) {
		throw LcyAsioException("The number of threads must be greater than 0");
	}
	
	for ( size_t i = 0; i < thread_num; ++i ) {
		threads_.push_back(new Thread());
	}
}

ThreadPool::~ThreadPool()
{
	if ( is_start_ ) {
		stop();
	}

	size_t size = threads_.size();
	for ( size_t i = 0; i < size; ++i ) {
		delete threads_[i];
	}
}

void ThreadPool::start()
{
	if ( is_start_ ) {
		return;
	}

	size_t size = threads_.size();

	for ( size_t i = 0; i < size; ++i ) {
		threads_[i]->start();
	}

	is_start_ = true;
}

void ThreadPool::stop()
{
	if ( !is_start_ ) {
		return;
	}

	size_t size = threads_.size();

	for ( size_t i = 0; i < size; ++i ) {
		threads_[i]->stop();
	}

	is_start_ = false;
}

IOContext& ThreadPool::nextContext()
{
	if ( !is_start_ ) {
		throw LcyAsioException("Thread pool is not start");
	}

	size_t size = threads_.size();

	size_t current = next_.load(std::memory_order_relaxed);
    size_t next;
    
    do {
        next = (current + 1) % size;
    } while ( !next_.compare_exchange_weak(
        current, next,
        std::memory_order_release,
        std::memory_order_relaxed
    ) );
    
	return threads_[current]->context();
}

}	// namespace asio
}	// namespace lcy
