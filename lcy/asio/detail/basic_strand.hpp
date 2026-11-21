#ifndef __LCY_ASIO_DETAIL_BASIC_STRAND_HPP__
#define __LCY_ASIO_DETAIL_BASIC_STRAND_HPP__

#include "lcy/noncopyable.hpp"
#include "lcy/asio/detail/reactor.hpp"
#include "lcy/asio/detail/strand_service.hpp"

namespace lcy
{
namespace asio
{
namespace detail
{

template <typename Executor>
class basic_strand :
    private noncopyable
{
public:
    typedef Executor executor_type;
    typedef strand_service::func_type func_type;

public:
    basic_strand(executor_type& executor) :
        executor_(executor), service_(use_service<strand_service>(executor))
    {
        service_.construct(impl_);
    }

    ~basic_strand()
    {
        service_.destroy(impl_);
    }

    void post(func_type func)
    {
        service_.post(impl_, std::move(func));
    }

private:
    typedef strand_service::strand_impl strand_impl;

    executor_type& executor_;
    strand_impl impl_;
    strand_service& service_;
};

}   // namespace detail
}   // namespace asio
}   // namespace lcy


#endif  // __LCY_ASIO_DETAIL_BASIC_STRAND_HPP__