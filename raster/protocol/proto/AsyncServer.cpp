/*
 * Copyright (C) 2017, Yeolar
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
