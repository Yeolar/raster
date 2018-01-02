/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <mutex>
#include <set>
#include <string>
#include <google/protobuf/service.h>

#include "raster/io/Cursor.h"
#include "raster/util/Function.h"

namespace rdd {

class PBRpcController : public google::protobuf::RpcController {
 public:
  PBRpcController() {}
  ~PBRpcController() override {}

  // Client-side
  void Reset() override;
  bool Failed() const override;
  std::string ErrorText() const override;
  void StartCancel() override;

  // Server side
  void SetFailed(const std::string& reason) override;
  bool IsCanceled() const override;
  void NotifyOnCancel(google::protobuf::Closure* closure) override;

  void parseFrom(io::Cursor& in);
  void serializeTo(IOBufQueue& out) const;

  void copyFrom(const PBRpcController& o);

  void setCanceled() { canceled_ = true; }
  void setStartCancel(VoidFunc&& cancelFunc);

  void complete();

 private:
  bool canceled_{false};
  bool failed_{false};
  std::string failedReason_;
  std::set<google::protobuf::Closure*> closures_;
  std::mutex closuresLock_;
  VoidFunc cancelFunc_;
};

} // namespace rdd
