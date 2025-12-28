#ifndef __LCY_RPC_CLOSURE_H__
#define __LCY_RPC_CLOSURE_H__

#include <memory>
#include <functional>
#include <google/protobuf/stubs/callback.h>

namespace lcy {
namespace rpc {

class Closure :
	public ::google::protobuf::Closure,
	public ::std::enable_shared_from_this<Closure>
{
public:
	typedef std::function<void ()> closure_op_type;
	friend std::shared_ptr<Closure> NewClosure(closure_op_type closure_op);

	~Closure();

	void Run() override;

private:
	Closure(closure_op_type closure_op);

private:
	closure_op_type closure_op_;
};

typedef std::function<void ()> closure_op_type;
std::shared_ptr<Closure> NewClosure(closure_op_type closure_op);

}	// namespace rpc
}	// namespace lcy

#endif // __LCY_RPC_CLOSURE_H__
