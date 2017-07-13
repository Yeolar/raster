/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <mutex>
#include <set>
#include <string>
#include <google/protobuf/service.h>
#include "rddoc/util/Function.h"

namespace rdd {

class PBRpcController : public google::protobuf::RpcController {
public:
  PBRpcController() {}

  // Client-side
  virtual void Reset();
  virtual bool Failed() const;
  virtual std::string ErrorText() const;
  virtual void StartCancel();

  // Server side
  virtual void SetFailed(const std::string& reason);
  virtual bool IsCanceled() const;
  virtual void NotifyOnCancel(google::protobuf::Closure* closure);

  void serializeTo(std::ostream& out) const;
  void parseFrom(std::istream& in);

  void setCanceled() { canceled_ = true; }

private:
  void setStartCancel(VoidFunc&& cancelFunc);

  void complete();

  bool canceled_{false};
  bool failed_{false};
  std::string failedReason_;
  std::set<google::protobuf::Closure*> closures_;
  std::mutex closuresLock_;
  VoidFunc cancelFunc_;
};

}
