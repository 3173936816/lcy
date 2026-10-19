namespace lcy
{
namespace asio
{
namespace detail
{

template <typename Endpoint>
class socket_service_base::accept_operation
{
public:
	accept_operation(socket_service_base* self,
					 socket_impl& source_impl,
					 socket_impl& target_impl,
					 Endpoint& endpoint,
					 accept_func_type func)
		: self_(self)
		, source_impl_(&source_impl)
		, target_impl_(&target_impl)
		, endpoint_(&endpoint)
		, func_(std::move(func)) {}

	void operator()(lcy::system::error_code ec, int) const
	{
		if ( !ec ) {
			int new_sockfd =
				socket_ops::accept(source_impl_->sockfd_, *endpoint_, ec);
			if ( new_sockfd != -1 ) {
				self_->assign(*target_impl_, new_sockfd, ec);
			}
		}

		func_(ec);
	}

private:
	socket_service_base* self_;
	socket_impl* source_impl_;
	socket_impl* target_impl_;
	Endpoint* endpoint_;
	accept_func_type func_;
};

}	// namespace detail
}	// namespace asio
}	// namespace lcy
