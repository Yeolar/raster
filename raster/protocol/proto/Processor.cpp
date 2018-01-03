/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/protocol/proto/Processor.h"

#include "raster/protocol/binary/Transport.h"
#include "raster/protocol/proto/Message.h"

namespace rdd {

void PBProcessor::run() {
  auto transport = event_->transport<BinaryTransport>();
  try {
    io::Cursor in(transport->body.get());
    int type = proto::readInt(in);
    switch (type) {
      case proto::REQUEST_MSG: {
        std::string callId;
        std::shared_ptr<google::protobuf::Message> request;
        const google::protobuf::MethodDescriptor* descriptor = nullptr;
        proto::parseRequestFrom(in, callId, descriptor, request);
        process(callId, descriptor, request);
        break;
      }
      case proto::CANCEL_MSG: {
        std::string callId;
        proto::parseCancelFrom(in, callId);
        cancel(callId);
        break;
      }
      default:
        RDDLOG(FATAL) << "unknown message type: " << type;
    }
  } catch (std::exception& e) {
    RDDLOG(WARN) << "catch exception: " << e.what();
  } catch (...) {
    RDDLOG(WARN) << "catch unknown exception";
  }
}

void PBProcessor::process(
    const std::string& callId,
    const google::protobuf::MethodDescriptor* method,
    const std::shared_ptr<google::protobuf::Message>& request) {
  google::protobuf::Service* service = server_->getService(method);
  if (service) {
    std::shared_ptr<PBRpcController> controller(new PBRpcController());
    std::shared_ptr<google::protobuf::Message>
      response(service->GetResponsePrototype(method).New());
    std::shared_ptr<PBAsyncServer::Handle>
      handle(new PBAsyncServer::Handle(
              callId, controller, request, response));
    server_->addHandle(handle);
    service->CallMethod(method,
                        controller.get(),
                        request.get(),
                        response.get(),
                        google::protobuf::NewCallback(
                            this, &PBProcessor::finish, handle));
  } else {
    PBRpcController controller;
    controller.SetFailed("failed to find service");
    sendResponse(callId, &controller, nullptr);
  }
}

void PBProcessor::cancel(const std::string& callId) {
  std::shared_ptr<PBAsyncServer::Handle> handle =
    server_->removeHandle(callId);
  RDDCHECK(handle) << "proto handle not available";
  PBRpcController controller;
  controller.setCanceled();
  sendResponse(callId, &controller, nullptr);
}

void PBProcessor::finish(std::shared_ptr<PBAsyncServer::Handle> handle) {
  RDDCHECK_EQ(handle, server_->removeHandle(handle->callId))
    << "proto handle not available";
  sendResponse(handle->callId,
               handle->controller.get(),
               handle->response.get());
}

void PBProcessor::sendResponse(
    const std::string& callId,
    PBRpcController* controller,
    google::protobuf::Message* response) {
  IOBufQueue out(IOBufQueue::cacheChainLength());
  proto::serializeResponse(callId, *controller, response, out);
  auto buf = out.move();
  auto transport = event_->transport<BinaryTransport>();
  transport->sendHeader(buf->computeChainDataLength());
  transport->sendBody(std::move(buf));
}

} // namespace rdd
