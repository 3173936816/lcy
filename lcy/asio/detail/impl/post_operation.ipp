namespace lcy
{
namespace asio
{
namespace detail
{

template <typename Function>
class socket_service_base::post_operation
{
private:
	typedef std::function<
		void (
			lcy::system::error_code ec
		)
	> internal_func_type;

public:
	template <typename... Args>
	post_operation(socket_service_base* self, Function func, Args&&... args) :
		self_(self)
	{
		internal_func_ = std::bind(std::move(func),
			std::placeholders::_1, std::forward<Args>(args)...);
	}

	void operator()(lcy::system::error_code ec, int) const
	{
		internal_func_(ec);
	}

private:
	socket_service_base* self_;
	internal_func_type internal_func_;
};

}	// namespace detail
}	// namespace asio
}	// namespace lcy
