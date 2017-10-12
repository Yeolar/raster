/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <memory>
#include <string>
#include <arpa/inet.h>
#include <google/protobuf/service.h>
#include "raster/io/event/Event.h"
#include "raster/net/NetUtil.h"
#include "raster/net/Socket.h"
#include "raster/protocol/proto/Encoding.h"
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

  void process(std::unique_ptr<IOBuf>& buf);

protected:
  virtual void send(
      std::unique_ptr<IOBuf>& buf,
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
  PBSyncRpcChannel(const Peer& peer) : peer_(peer), socket_(0) {}

  virtual ~PBSyncRpcChannel() {}

  void setTimeout(const TimeoutOption& opt) {
    socket_.setConnTimeout(opt.ctimeout);
    socket_.setRecvTimeout(opt.rtimeout);
    socket_.setSendTimeout(opt.wtimeout);
  }

  void open() {
    socket_.setReuseAddr();
    socket_.setTCPNoDelay();
    socket_.connect(peer_);
  }

  bool isOpen() {
    return socket_.isConnected();
  }

  void close() {
    socket_.close();
  }

private:
  virtual void send(
      std::unique_ptr<IOBuf>& buf,
      std::function<void(bool, const std::string&)> resultCb) {
    uint32_t h = htonl(buf->computeChainDataLength());
    socket_.send(&h, sizeof(uint32_t));
    io::Cursor cursor(buf.get());
    while (!cursor.isAtEnd()) {
      auto p = cursor.peek();
      socket_.send((uint8_t*)p.first, p.second);
    }
    resultCb(true, "");

    uint32_t n;
    socket_.recv(&n, sizeof(uint32_t));
    std::unique_ptr<IOBuf> data(IOBuf::create(Protocol::CHUNK_SIZE));
    io::Appender appender(data.get(), Protocol::CHUNK_SIZE);
    n = ntohl(n);
    appender.ensure(n);
    socket_.recv(appender.writableData(), n);
    process(data);
  }

  Peer peer_;
  Socket socket_;
};

class PBAsyncRpcChannel : public PBRpcChannel {
public:
  PBAsyncRpcChannel(Event* event) : event_(event) {}

  virtual ~PBAsyncRpcChannel() {}

private:
  virtual void send(
      std::unique_ptr<IOBuf>& buf,
      std::function<void(bool, const std::string&)> resultCb) {
    proto::encodeData(event_->wbuf(), buf);
    resultCb(true, "");
  }

  Event* event_;
};

} // namespace rdd
