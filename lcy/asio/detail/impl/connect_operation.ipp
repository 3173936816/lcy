namespace lcy
{
namespace asio
{
namespace detail
{

template <typename Endpoint>
class socket_service_base::connect_operation
{
public:
	connect_operation(socket_service_base* self,
					  socket_impl& impl,
					  connect_func_type func)
		: self_(self)
		, impl_(&impl)
		, func_(std::move(func)) {}

	void operator()(lcy::system::error_code ec, int) const
	{
		if ( !ec ) {
			int error = 0;
			socklen_t len = sizeof(error);

			if ( !socket_ops::getsockopt(impl_->sockfd_,
				SOL_SOCKET, SO_ERROR, &error, &len, ec) )
			{
				ec = lcy::system::error_code(error);
			}
		}

		func_(ec);
	}

private:
	socket_service_base* self_;
	socket_impl* impl_;
	accept_func_type func_;
};

}	// namespace detail
}	// namespace asio
}	// namespace lcy
