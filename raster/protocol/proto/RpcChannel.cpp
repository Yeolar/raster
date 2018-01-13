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

#include "raster/protocol/proto/RpcChannel.h"

#include <sstream>

#include "raster/protocol/proto/Message.h"
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
  handles_.update(std::make_pair(callId, handle));
  IOBufQueue out(IOBufQueue::cacheChainLength());
  proto::serializeRequest(callId, *method, *request, out);
  send(out.move(), std::bind(&PBRpcChannel::messageSent, this, _1, _2, callId));
}

void PBRpcChannel::messageSent(
    bool success,
    const std::string& reason,
    std::string callId) {
  if (!success) {
    std::shared_ptr<Handle> handle = handles_.erase_get(callId);
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
  if (handles_.count(callId) == 0) {
    return;
  }
  IOBufQueue out(IOBufQueue::cacheChainLength());
  proto::serializeCancel(callId, out);
  send(out.move(), std::bind(&PBRpcChannel::messageSent, this, _1, _2, callId));
}

void PBRpcChannel::process(const std::unique_ptr<IOBuf>& buf) {
  try {
    io::Cursor in(buf.get());
    int type = proto::readInt(in);
    switch (type) {
      case proto::RESPONSE_MSG: {
        std::string callId;
        PBRpcController controller;
        std::shared_ptr<google::protobuf::Message> response;
        proto::parseResponseFrom(in, callId, controller, response);
        std::shared_ptr<Handle> handle = handles_.erase_get(callId);
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

void PBSyncRpcChannel::open() {
  socket_ = Socket::createSyncSocket();
  socket_->setConnTimeout(timeout_.ctimeout);
  socket_->setRecvTimeout(timeout_.rtimeout);
  socket_->setSendTimeout(timeout_.wtimeout);
  socket_->connect(peer_);
}

bool PBSyncRpcChannel::isOpen() {
  return socket_->isConnected();
}

void PBSyncRpcChannel::close() {
  socket_->close();
}

void PBSyncRpcChannel::send(
    std::unique_ptr<IOBuf> buf,
    std::function<void(bool, const std::string&)> resultCb) {
  transport_.sendHeader(buf->computeChainDataLength());
  transport_.sendBody(std::move(buf));
  transport_.writeData(socket_.get());
  resultCb(true, "");

  transport_.readData(socket_.get());
  process(transport_.body);
}

void PBAsyncRpcChannel::send(
    std::unique_ptr<IOBuf> buf,
    std::function<void(bool, const std::string&)> resultCb) {
  auto transport = event_->transport<BinaryTransport>();
  transport->sendHeader(buf->computeChainDataLength());
  transport->sendBody(std::move(buf));
  resultCb(true, "");
}

} // namespace rdd
