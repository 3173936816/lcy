namespace lcy {
namespace rpc {

template <class request_type, class response_type> 
Context::Context()
{
}

template <class request_type, class response_type> 
Context::~Context()
{
}

template <class request_type, class response_type> 
Controller* Context::controller()
{
	return &controller_;
}

template <class request_type, class response_type> 
request_type* Context::request()
{
	return &request_;
}

template <class request_type, class response_type> 
response_type* Context::response()
{
	return &response_;
}

template <class request_type, class response_type> 
std::shared_ptr<Context<request_type, response_type> > NewContext()
{
	return std::make_shared<Context<request_type, response_type> >();
}

}	// namespace rpc
}	// namespace lcy
