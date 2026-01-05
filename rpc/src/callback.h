#ifndef __LCY_RPC_CALLBACK_H__
#define __LCY_RPC_CALLBACK_H__

#include <functional>

namespace lcy {
namespace rpc {

class LambdaClosure : public ::google::protobuf::Closure {
public:
    explicit LambdaClosure(std::function<void()> func) : func_(std::move(func)) {}
    
    void Run() override {
        func_();
    }
    
private:
    std::function<void()> func_;
};

}	// namespace rpc
}	// namespace lcy

#endif // __LCY_RPC_CALLBACK_H__
