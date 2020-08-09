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

#include "raster/protocol/proto/Processor.h"

#include "raster/protocol/binary/Transport.h"
#include "raster/protocol/proto/Message.h"

namespace raster {

void PBProcessor::run() {
  auto transport = event_->transport<BinaryTransport>();
  try {
    acc::io::Cursor in(transport->body.get());
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
        ACCLOG(FATAL) << "unknown message type: " << type;
    }
  } catch (std::exception& e) {
    ACCLOG(WARN) << "catch exception: " << e.what();
  } catch (...) {
    ACCLOG(WARN) << "catch unknown exception";
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
  ACCCHECK(handle) << "proto handle not available";
  PBRpcController controller;
  controller.setCanceled();
  sendResponse(callId, &controller, nullptr);
}

void PBProcessor::finish(std::shared_ptr<PBAsyncServer::Handle> handle) {
  ACCCHECK_EQ(handle, server_->removeHandle(handle->callId))
    << "proto handle not available";
  sendResponse(handle->callId,
               handle->controller.get(),
               handle->response.get());
}

void PBProcessor::sendResponse(
    const std::string& callId,
    PBRpcController* controller,
    google::protobuf::Message* response) {
  acc::IOBufQueue out(acc::IOBufQueue::cacheChainLength());
  proto::serializeResponse(callId, *controller, response, out);
  auto buf = out.move();
  auto transport = event_->transport<BinaryTransport>();
  transport->sendHeader(buf->computeChainDataLength());
  transport->sendBody(std::move(buf));
}

} // namespace raster
