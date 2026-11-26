namespace lcy
{
namespace asio
{
namespace detail
{

template <
	typename Endpoint,
	typename ConstBufferView
>
class socket_service_base::send_to_operation
{
public:
	send_to_operation(socket_service_base* self,
		socket_impl& impl, Endpoint endpoint,
		ConstBufferView view, write_func_type func) :
		self_(self),
		impl_(&impl),
		endpoint_(endpoint),
		view_(view),
		func_(std::move(func)) {}

	void operator()(lcy::system::error_code ec, int) const
	{
		ssize_t transferred_bytes = 0;

		if ( !ec ) {
			transferred_bytes = socket_ops::sendto(impl_->sockfd_,
				endpoint_, view_, ec);
			if ( transferred_bytes == -1 ) {
				transferred_bytes = 0; 
			}
		}

		func_(ec, transferred_bytes);
	}

private:
	socket_service_base* self_;
	socket_impl* impl_;
	Endpoint endpoint_;
	ConstBufferView view_;
	write_func_type func_;
};

}	// namespace detail
}	// namespace asio
}	// namespace lcy
