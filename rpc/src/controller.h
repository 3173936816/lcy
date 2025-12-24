#ifndef __LCY_RPC_CONTROLLER_H__
#define __LCY_RPC_CONTROLLER_H__

#include <string>
#include <google/protobuf/service.h>

namespace lcy {
namespace rpc {

class Controller :
	public ::google::protobuf::RpcController
{
public:
	Controller();
	~Controller();

	void Reset() override;
  	bool Failed() const override;
  	std::string ErrorText() const override;
  	void SetFailed(const std::string& reason) override;

private:
  	void StartCancel() override;
  	bool IsCanceled() const override;
  	void NotifyOnCancel(::google::protobuf::Closure* callback) override;

private:
	bool failed_;
    std::string errmsg_;
};

}	// namespace rpc
}	// namespace lcy

#endif // __LCY_RPC_CONTROLLER_H__
