/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <memory>
#include <string>
#include <google/protobuf/service.h>

#include "raster/io/event/Event.h"
#include "raster/net/NetUtil.h"
#include "raster/net/Socket.h"
#include "raster/protocol/binary/Transport.h"
#include "raster/thread/LockedMap.h"

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

  void process(const std::unique_ptr<IOBuf>& buf);

 protected:
  virtual void send(
      std::unique_ptr<IOBuf> buf,
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

  LockedMap<std::string, std::shared_ptr<Handle>> handles_;
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
      std::unique_ptr<IOBuf> buf,
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
      std::unique_ptr<IOBuf> buf,
      std::function<void(bool, const std::string&)> resultCb) override;

  Event* event_;
};

} // namespace rdd
