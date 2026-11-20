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
    private service_registry_base<
        strand_service
    >
{
public:
    typedef std::function<void()> func_type;

public:
    struct strand_impl
    {
        conditionally_mutex cond_mutex_;
        std::list<operation> op_list_;

        explicit strand_impl(bool enable_lock) :
            cond_mutex_(enable_lock) {}
    };

private:
    class strand_operation
    {
    public:
        strand_operation(strand_service* self, func_type func) :
            self_(self), func_(std::move(func)) {}

        void operator()(lcy::system::error_code, int) const
        {
            func_();
        }

    private:
        strand_service* self_;
        func_type func_;
    };

public:
    explicit strand_service(reactor& r) :

private:
    reactor* reactor_;
    conditionally_mutex strand_cond_mutex_;
    object_pool<strand_impl> strand_datas_;
};

}   // namespace detail
}   // namespace asio
}   // namespace lcy

#endif  // __LCY_ASIO_DETAIL_STRAND_SERVICE_HPP__