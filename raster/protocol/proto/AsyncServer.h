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

#include <map>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/service.h>

#include "accelerator/thread/LockedMap.h"
#include "raster/net/Service.h"
#include "raster/protocol/proto/RpcController.h"

namespace rdd {

class PBAsyncServer : public Service {
 public:
  PBAsyncServer(acc::StringPiece name) : Service(name) {}
  ~PBAsyncServer() override {}

  void makeChannel(int port, const TimeoutOption& timeout) override;

  void addService(
      const std::shared_ptr<google::protobuf::Service>& service);

  google::protobuf::Service* getService(
      const google::protobuf::MethodDescriptor* method);

  struct Handle {
    Handle(const std::string& callId_,
           const std::shared_ptr<PBRpcController>& controller_,
           const std::shared_ptr<google::protobuf::Message>& request_,
           const std::shared_ptr<google::protobuf::Message>& response_)
      : callId(callId_),
        controller(controller_),
        request(request_),
        response(response_) {}

    std::string callId;
    std::shared_ptr<PBRpcController> controller;
    std::shared_ptr<google::protobuf::Message> request;
    std::shared_ptr<google::protobuf::Message> response;
  };

  void addHandle(std::shared_ptr<Handle>& handle);

  std::shared_ptr<Handle> removeHandle(const std::string& callId);

 private:
  std::map<
    const google::protobuf::ServiceDescriptor*,
    std::shared_ptr<google::protobuf::Service>> services_;
  acc::LockedMap<std::string, std::shared_ptr<Handle>> handles_;
};

} // namespace rdd
