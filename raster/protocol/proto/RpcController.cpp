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

#include "raster/protocol/proto/RpcController.h"

#include "raster/protocol/proto/Message.h"

namespace rdd {

void PBRpcController::Reset() {
  canceled_ = false;
  failed_ = false;
  failedReason_ = "";
  cancelFunc_ = nullptr;
  std::lock_guard<std::mutex> guard(closuresLock_);
  closures_.clear();
}

bool PBRpcController::Failed() const {
  return failed_;
}

std::string PBRpcController::ErrorText() const {
  return failedReason_;
}

void PBRpcController::StartCancel() {
  if (cancelFunc_) {
    cancelFunc_();
  }
}

void PBRpcController::SetFailed(const std::string& reason) {
  failed_ = true;
  failedReason_ = reason;
}

bool PBRpcController::IsCanceled() const {
  return canceled_;
}

void PBRpcController::NotifyOnCancel(google::protobuf::Closure* closure) {
  if (!closure) {
    return;
  }
  if (canceled_) {
    closure->Run();
  } else {
    std::lock_guard<std::mutex> guard(closuresLock_);
    closures_.insert(closure);
  }
}

void PBRpcController::parseFrom(acc::io::Cursor& in) {
  canceled_ = (proto::readChar(in) == 'Y');
  failed_ = (proto::readChar(in) == 'Y');
  if (failed_) {
    failedReason_ = proto::readString(in);
  }
}

void PBRpcController::serializeTo(acc::IOBufQueue& out) const {
  proto::writeChar(canceled_ ? 'Y' : 'N', out);
  proto::writeChar(failed_ ? 'Y' : 'N', out);
  if (failed_) {
    proto::writeString(failedReason_, out);
  }
}

void PBRpcController::copyFrom(const PBRpcController& o) {
  if (o.failed_) {
    SetFailed(o.ErrorText());
  }
  canceled_ = o.canceled_;
}

void PBRpcController::setStartCancel(acc::VoidFunc&& cancelFunc) {
  cancelFunc_ = std::move(cancelFunc);
}

void PBRpcController::complete() {
  std::set<google::protobuf::Closure*> closures;
  {
    std::lock_guard<std::mutex> guard(closuresLock_);
    closures.swap(closures_);
  }
  for (auto& c : closures) {
    c->Run();
  }
}

} // namespace rdd
