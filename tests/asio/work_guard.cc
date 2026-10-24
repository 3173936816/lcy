#include <vector>
#include <memory>
#include <iostream>

#include "lcy/asio/work_guard.hpp"
#include "lcy/asio/signal_set.hpp"

class thread_pool
{
public:
    thread_pool(std::size_t thread_num) :
        index_(0)
    {
        for ( std::size_t i = 0; i < thread_num; ++i ) {
            workers.emplace_back(new lcy::asio::io_context());
            work_guards.emplace_back(new lcy::asio::work_guard(*workers[i]));
        }
    }

    void start()
    {
        for ( std::size_t i = 0; i < workers.size(); ++i ) {
            threads_.emplace_back(new std::thread([i, this](){
                std::cout << "thread " << i << " start" << std::endl;
                workers[i]->run();
                std::cout << "thread " << i << " exit" << std::endl;
            }));
        }
    }

    void stop()
    {
        for ( auto& ptr : work_guards ) {
            ptr->reset();
        }

        for ( auto& th : threads_ ) {
            th->join();
        }
    }

    void post(std::function<void()> func)
    {
        lcy::asio::post(*workers[index_], std::move(func));
        index_ = (index_ + 1) % workers.size();
    }

private:
    int index_;
    std::vector<std::unique_ptr<std::thread> > threads_;
    std::vector<std::unique_ptr<lcy::asio::io_context> > workers;
    std::vector<std::unique_ptr<lcy::asio::work_guard> > work_guards;
};

void test_work_guard()
{
    thread_pool pool(5);
    pool.start();

    sleep(3);

    for ( std::size_t i = 0; i < 10000; ++i ) {
        pool.post([i](){
            std::cout << "task " << i << std::endl;
        });
    }

    sleep(5);

    pool.stop();
}

int main()
{
    test_work_guard();

    return 0;
}