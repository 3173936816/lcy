#include "lcy/rpc/src/closure.h"

namespace lcy {
namespace rpc {

Closure::Closure(closure_op_type closure_op) :
	closure_op_(std::move(closure_op))
{
}

Closure::~Closure()
{
}

void Closure::Run()
{
	closure_op_();
}

std::shared_ptr<Closure> NewClosure(closure_op_type closure_op)
{
	return std::shared_ptr<Closure>(new Closure(std::move(closure_op)));
}

}	// namespace rpc
}	// namespace lcy
