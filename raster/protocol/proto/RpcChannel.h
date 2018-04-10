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

#include <memory>
#include <string>
#include <google/protobuf/service.h>

#include "raster/event/Event.h"
#include "raster/net/NetUtil.h"
#include "raster/net/Socket.h"
#include "raster/protocol/binary/Transport.h"
#include "accelerator/thread/LockedMap.h"

namespace rdd {

class PBRpcChannel : public google::protobuf::RpcChannel {
 public:
  ~PBRpcChannel() override {}

  void CallMethod(
      const google::protobuf::MethodDescriptor* method,
      google::protobuf::RpcController* controller,
      const google::protobuf::Message* request,
      google::protobuf::Message* response,
      google::protobuf::Closure* done) override;

  void process(const std::unique_ptr<acc::IOBuf>& buf);

 protected:
  virtual void send(
      std::unique_ptr<acc::IOBuf> buf,
      std::function<void(bool, const std::string&)> resultCb) = 0;

 private:
  struct Handle {
    Handle(google::protobuf::RpcController* controller_,
           google::protobuf::Message* response_,
           google::protobuf::Closure* done_)
      : controller(controller_),
        response(response_),
        done(done_) {}

    google::protobuf::RpcController* controller;
    google::protobuf::Message* response;
    google::protobuf::Closure* done;
  };

  void messageSent(bool success, const std::string& reason, std::string callId);

  void startCancel(std::string callId);

  acc::LockedMap<std::string, std::shared_ptr<Handle>> handles_;
};

class PBSyncRpcChannel : public PBRpcChannel {
 public:
  PBSyncRpcChannel(const Peer& peer, const TimeoutOption& timeout)
    : peer_(peer), timeout_(timeout) {}

  ~PBSyncRpcChannel() override {}

  void open();

  bool isOpen();

  void close();

 private:
  void send(
      std::unique_ptr<acc::IOBuf> buf,
      std::function<void(bool, const std::string&)> resultCb) override;

  Peer peer_;
  TimeoutOption timeout_;
  std::unique_ptr<Socket> socket_;
  BinaryTransport transport_;
};

class PBAsyncRpcChannel : public PBRpcChannel {
 public:
  PBAsyncRpcChannel(Event* event) : event_(event) {}
  ~PBAsyncRpcChannel() override {}

 private:
  void send(
      std::unique_ptr<acc::IOBuf> buf,
      std::function<void(bool, const std::string&)> resultCb) override;

  Event* event_;
};

} // namespace rdd
