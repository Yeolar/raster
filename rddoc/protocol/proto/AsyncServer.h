/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <map>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/service.h>
#include "rddoc/net/Service.h"
#include "rddoc/protocol/proto/Protocol.h"
#include "rddoc/protocol/proto/RpcController.h"
#include "rddoc/util/LockedMap.h"

namespace rdd {

class PBAsyncServer : public Service {
public:
  virtual void makeChannel(int port, const TimeoutOption& timeout_opt);

  void addService(
      const std::shared_ptr<google::protobuf::Service>& service) {
    if (service) {
      services_[service->GetDescriptor()] = service;
    }
  }

  google::protobuf::Service* getService(
      const google::protobuf::MethodDescriptor* method) {
    return services_[method->service()].get();
  }

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

  void addHandle(std::shared_ptr<Handle>& handle) {
    handles_.insert(handle->callId, handle);
  }

  std::shared_ptr<Handle> removeHandle(const std::string& callId) {
    return handles_.erase(callId);
  }

private:
  std::map<const google::protobuf::ServiceDescriptor*,
           std::shared_ptr<google::protobuf::Service>> services_;
  LockedMap<std::string, std::shared_ptr<Handle>> handles_;
};

}

