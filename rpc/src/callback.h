#ifndef __LCY_RPC_CALLBACK_H__
#define __LCY_RPC_CALLBACK_H__

#include <functional>

namespace lcy {
namespace rpc {

class LambdaClosure : public ::google::protobuf::Closure {
public:
	friend LambdaClosure* NewLambdaClosure(std::function<void()> func, bool owner);
    
    void Run() override {
        func_();
		if ( owner_ ) delete this;
    }

private:
    explicit LambdaClosure(std::function<void()> func, bool owner = false)
		: owner_(owner), func_(std::move(func)) {}
    
private:
	bool owner_; 
    std::function<void()> func_;
};

LambdaClosure* NewLambdaClosure(std::function<void()> func, bool owner = false)
{
	return new LambdaClosure(std::move(func), owner);
}

}	// namespace rpc
}	// namespace lcy

#endif // __LCY_RPC_CALLBACK_H__
