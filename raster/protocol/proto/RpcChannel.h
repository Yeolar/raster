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
#include "raster/protocol/proto/Message.h"
#include "raster/util/LockedMap.h"

namespace rdd {

class PBRpcChannel : public google::protobuf::RpcChannel {
public:
  PBRpcChannel() {}
  virtual ~PBRpcChannel() {}

  virtual void CallMethod(
      const google::protobuf::MethodDescriptor* method,
      google::protobuf::RpcController* controller,
      const google::protobuf::Message* request,
      google::protobuf::Message* response,
      google::protobuf::Closure* done);

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
  PBSyncRpcChannel(const Peer& peer) : peer_(peer) {}

  virtual ~PBSyncRpcChannel() {}

  void setTimeout(const TimeoutOption& opt) {
    socket_->setConnTimeout(opt.ctimeout);
    socket_->setRecvTimeout(opt.rtimeout);
    socket_->setSendTimeout(opt.wtimeout);
  }

  void open() {
    socket_ = Socket::createSyncSocket();
    socket_->connect(peer_);
  }

  bool isOpen() {
    return socket_->isConnected();
  }

  void close() {
    socket_->close();
  }

private:
  virtual void send(
      std::unique_ptr<IOBuf> buf,
      std::function<void(bool, const std::string&)> resultCb) {
    transport_.sendHeader(buf->computeChainDataLength());
    transport_.sendBody(std::move(buf));
    transport_.writeData(socket_.get());
    resultCb(true, "");

    transport_.readData(socket_.get());
    process(transport_.body);
  }

  Peer peer_;
  std::unique_ptr<Socket> socket_;
  BinaryTransport transport_;
};

class PBAsyncRpcChannel : public PBRpcChannel {
public:
  PBAsyncRpcChannel(Event* event) : event_(event) {}

  virtual ~PBAsyncRpcChannel() {}

private:
  virtual void send(
      std::unique_ptr<IOBuf> buf,
      std::function<void(bool, const std::string&)> resultCb) {
    auto transport = event_->transport<BinaryTransport>();
    transport->sendHeader(buf->computeChainDataLength());
    transport->sendBody(std::move(buf));
    resultCb(true, "");
  }

  Event* event_;
};

} // namespace rdd
