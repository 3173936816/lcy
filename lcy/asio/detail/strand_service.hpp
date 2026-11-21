#ifndef __LCY_ASIO_DETAIL_STRAND_SERVICE_HPP__
#define __LCY_ASIO_DETAIL_STRAND_SERVICE_HPP__

#include <list>
#include <functional>

#include "lcy/object_pool.hpp"
#include "lcy/asio/detail/error.hpp"
#include "lcy/asio/detail/reactor.hpp"
#include "lcy/asio/detail/operation.hpp"
#include "lcy/asio/detail/conditionally_mutex.hpp"
#include "lcy/asio/detail/service_registry_base.hpp"

namespace lcy
{
namespace asio
{
namespace detail
{

class strand_service :
    public service_registry_base<
        strand_service
    >
{
public:
    typedef std::function<void()> func_type;

private:
    struct internal_strand
    {
        conditionally_mutex cond_mutex_;
        bool executing_;
        std::list<func_type> func_list_;

        explicit internal_strand(bool enable_lock) :
            cond_mutex_(enable_lock), executing_(false) {}
    };

public:
    struct strand_impl
    {
        internal_strand* data_;
    };

private:
    class strand_operation
    {
    public:
        strand_operation(strand_service* self, strand_impl& impl) :
            self_(self), impl_(&impl) {}

        void operator()(lcy::system::error_code, int) const
        {
            func_type func;
            {
                std::lock_guard<conditionally_mutex> locker(impl_->data_->cond_mutex_);

                if ( !impl_->data_->func_list_.empty() ) {
                    func = std::move(impl_->data_->func_list_.front());
                    impl_->data_->func_list_.pop_front();
                }
            }

            if ( func ) {
                func();
            }

            bool has_more = false;
            {
                std::lock_guard<conditionally_mutex> locker(impl_->data_->cond_mutex_);

                if ( !impl_->data_->func_list_.empty() ) {
                    has_more = true;
                } else {
                    impl_->data_->executing_ = false;
                }
            }

            if ( has_more ) {
                self_->do_execute(*impl_);
            }
        }

    private:
        strand_service* self_;
        strand_impl* impl_;
    };

public:
    explicit strand_service(reactor& r) :
        reactor_(&r), strand_cond_mutex_(r.concurrency_hint()) {}

    void construct(strand_impl& impl)
    {
        impl.data_ = alloc_internal_strand();
    }

    void destroy(strand_impl& impl)
    {
        free_internal_strand(impl.data_);
        impl.data_ = 0;
    }

    void post(strand_impl& impl, func_type func)
    {
        bool need_execute = false;
        {
            std::lock_guard<conditionally_mutex> lock(impl.data_->cond_mutex_);

            impl.data_->func_list_.push_back(std::move(func));
            if ( !impl.data_->executing_ ) {
                impl.data_->executing_ = true;
                need_execute = true;
            }
        }

        if ( need_execute ) {
            do_execute(impl);
        }
    }

    void cleanup() override
    {
        std::list<internal_strand*> strand_list;

        {
            std::lock_guard<conditionally_mutex> locker(strand_cond_mutex_);
            strand_datas_.get_live_objects(strand_list);
        }

        for ( auto s : strand_list ) {
            std::lock_guard<conditionally_mutex> locker(s->cond_mutex_);
            s->executing_ = false;
            s->func_list_.clear();
        }
    }

private:
    internal_strand* alloc_internal_strand()
    {
        std::lock_guard<conditionally_mutex> locker(strand_cond_mutex_);
        return strand_datas_.alloc(reactor_->concurrency_hint() != 1);
    }

    void free_internal_strand(internal_strand* s)
    {
        std::lock_guard<conditionally_mutex> locker(strand_cond_mutex_);
        strand_datas_.free(s);
    }

    void do_execute(strand_impl& impl)
    {
        strand_operation strand_op(this, impl);
        reactor_->post_to_scheduler(operation(std::move(strand_op)));
    }

private:
    reactor* reactor_;
    conditionally_mutex strand_cond_mutex_;
    object_pool<internal_strand> strand_datas_;
};

}   // namespace detail
}   // namespace asio
}   // namespace lcy

#endif  // __LCY_ASIO_DETAIL_STRAND_SERVICE_HPP__