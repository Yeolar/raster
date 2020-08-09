/*
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <google/protobuf/service.h>

#include "raster/net/Processor.h"
#include "raster/protocol/proto/AsyncServer.h"

namespace raster {

class PBProcessor : public Processor {
 public:
  PBProcessor(Event* event, PBAsyncServer* server)
    : Processor(event), server_(server) {}

  ~PBProcessor() override {}

  void run() override;

 private:
  void process(
      const std::string& callId,
      const google::protobuf::MethodDescriptor* method,
      const std::shared_ptr<google::protobuf::Message>& request);

  void cancel(const std::string& callId);

  void finish(std::shared_ptr<PBAsyncServer::Handle> handle);

  void sendResponse(
      const std::string& callId,
      PBRpcController* controller,
      google::protobuf::Message* response);

  PBAsyncServer* server_;
};

class PBProcessorFactory : public ProcessorFactory {
 public:
  PBProcessorFactory(PBAsyncServer* server) : server_(server) {}
  ~PBProcessorFactory() override {}

  std::unique_ptr<Processor> create(Event* event) override {
    return std::make_unique<PBProcessor>(event, server_);
  }

 private:
  PBAsyncServer* server_;
};

} // namespace raster
