namespace lcy
{
namespace asio
{
namespace detail
{

template <typename ConstBufferView>
class socket_service_base::send_operation
{
public:
	send_operation(socket_service_base* self,
		socket_impl& impl, ConstBufferView view,
		write_func_type func) :
		self_(self),
		impl_(&impl),
		view_(view),
		func_(std::move(func)) {}

	void operator()(lcy::system::error_code ec, int) const
	{
		int transferred_bytes = 0;

		if ( !ec ) {
			transferred_bytes = socket_ops::send(impl_->sockfd_, view_, ec);

			if ( transferred_bytes == -1 ) {
				transferred_bytes = 0;
			}
		}

		func_(ec, transferred_bytes);
	}

private:
	socket_service_base* self_;
	socket_impl* impl_;
	ConstBufferView view_;
	write_func_type func_;
};

}	// namespace detail
}	// namespace asio
}	// namespace lcy
