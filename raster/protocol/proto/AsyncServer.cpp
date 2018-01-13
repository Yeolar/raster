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

#include "raster/protocol/proto/AsyncServer.h"

#include "raster/protocol/binary/Transport.h"
#include "raster/protocol/proto/Processor.h"

namespace rdd {

void PBAsyncServer::makeChannel(int port, const TimeoutOption& timeout) {
  Peer peer;
  peer.setFromLocalPort(port);
  channel_ = std::make_shared<Channel>(
      peer,
      timeout,
      make_unique<BinaryTransportFactory>(),
      make_unique<PBProcessorFactory>(this));
}

void PBAsyncServer::addService(
    const std::shared_ptr<google::protobuf::Service>& service) {
  if (service) {
    services_[service->GetDescriptor()] = service;
  }
}

google::protobuf::Service* PBAsyncServer::getService(
    const google::protobuf::MethodDescriptor* method) {
  return services_[method->service()].get();
}

void PBAsyncServer::addHandle(std::shared_ptr<Handle>& handle) {
  handles_.insert(std::make_pair(handle->callId, handle));
}

std::shared_ptr<PBAsyncServer::Handle>
PBAsyncServer::removeHandle(const std::string& callId) {
  return handles_.erase_get(callId);
}

} // namespace rdd
