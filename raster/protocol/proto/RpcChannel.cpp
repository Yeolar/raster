/*
 * Copyright (C) 2017, Yeolar
 */

#include <sstream>
#include "raster/protocol/proto/Encoding.h"
#include "raster/protocol/proto/Message.h"
#include "raster/protocol/proto/RpcChannel.h"
#include "raster/protocol/proto/RpcController.h"
#include "raster/util/Uuid.h"

namespace rdd {

using namespace std::placeholders;

void PBRpcChannel::CallMethod(
    const google::protobuf::MethodDescriptor* method,
    google::protobuf::RpcController* controller,
    const google::protobuf::Message* request,
    google::protobuf::Message* response,
    google::protobuf::Closure* done) {
  std::string callId = uuidGenerateTime();
  if (controller) {
    PBRpcController* c = dynamic_cast<PBRpcController*>(controller);
    if (c) {
      c->setStartCancel(std::bind(&PBRpcChannel::startCancel, this, callId));
    }
  }
  std::shared_ptr<Handle> handle(new Handle(controller, response, done));
  handles_[callId] = handle;
  std::unique_ptr<IOBuf> buf(IOBuf::create(Protocol::CHUNK_SIZE));
  io::Appender out(buf.get(), Protocol::CHUNK_SIZE);
  proto::serializeRequest(callId, *method, *request, out);
  send(buf, std::bind(&PBRpcChannel::messageSent, this, _1, _2, callId));
}

void PBRpcChannel::messageSent(
    bool success,
    const std::string& reason,
    std::string callId) {
  if (!success) {
    std::shared_ptr<Handle> handle = handles_.erase(callId);
    if (handle) {
      PBRpcController* c = dynamic_cast<PBRpcController*>(handle->controller);
      if (c) {
        c->SetFailed(reason);
      }
      if (handle->done) {
        handle->done->Run();
      }
      if (c) {
        c->complete();
      }
    }
  }
}

void PBRpcChannel::startCancel(std::string callId) {
  if (handles_.contains(callId)) {
    std::unique_ptr<IOBuf> buf(IOBuf::create(Protocol::CHUNK_SIZE));
    io::Appender out(buf.get(), Protocol::CHUNK_SIZE);
    proto::serializeCancel(callId, out);
    send(buf,
         std::bind(&PBRpcChannel::messageSent, this, _1, _2, callId));
  }
}

void PBRpcChannel::process(std::unique_ptr<IOBuf>& buf) {
  try {
    io::RWPrivateCursor in(buf.get());
    int type = proto::readInt(in);
    switch (type) {
      case proto::RESPONSE_MSG:
      {
        std::string callId;
        PBRpcController controller;
        std::shared_ptr<google::protobuf::Message> response;
        proto::parseResponseFrom(in, callId, controller, response);
        std::shared_ptr<Handle> handle = handles_.erase(callId);
        if (handle) {
          PBRpcController* c =
            dynamic_cast<PBRpcController*>(handle->controller);
          if (c) {
            c->copyFrom(controller);
          }
          if (handle->response && response &&
              handle->response->GetDescriptor() == response->GetDescriptor()) {
            handle->response->CopyFrom(*response);
          }
          if (handle->done) {
            handle->done->Run();
          }
          if (c) {
            c->complete();
          }
        }
      }
        break;
    }
  } catch (std::exception& e) {
    RDDLOG(WARN) << "catch exception: " << e.what();
  } catch (...) {
    RDDLOG(WARN) << "catch unknown exception";
  }
}

} // namespace rdd
