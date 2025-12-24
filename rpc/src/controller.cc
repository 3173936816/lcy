#include "lcy/rpc/controller.h"

namespace lcy {
namespace rpc {

Controller::Controller() :
	failed_(true),
	errmsg_()
{
}

Controller::~Controller()
{
}

void Controller::Reset()
{
	failed_ = true;
	errmsg_ = {};
}

bool Controller::Failed() const
{
	return failed_;
}

std::string Controller::ErrorText() const
{
	return errmsg_;
}

void Controller::SetFailed(const std::string& reason)
{
	errmsg_ = reason;
}

void Controller::StartCancel()
{
}

bool Controller::IsCanceled() const
{
	return false;
}

void Controller::NotifyOnCancel(::google::protobuf::Closure* callback)
{
}

}	// namespace rpc
}	// namespace lcy
