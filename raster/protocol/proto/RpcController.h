/*
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <mutex>
#include <set>
#include <string>
#include <google/protobuf/service.h>

#include "accelerator/Function.h"
#include "accelerator/io/Cursor.h"

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

  void parseFrom(acc::io::Cursor& in);
  void serializeTo(acc::IOBufQueue& out) const;

  void copyFrom(const PBRpcController& o);

  void setCanceled() { canceled_ = true; }
  void setStartCancel(acc::VoidFunc&& cancelFunc);

  void complete();

 private:
  bool canceled_{false};
  bool failed_{false};
  std::string failedReason_;
  std::set<google::protobuf::Closure*> closures_;
  std::mutex closuresLock_;
  acc::VoidFunc cancelFunc_;
};

} // namespace rdd
