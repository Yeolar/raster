/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <sstream>
#include <google/protobuf/service.h>
#include "rddoc/io/event/Event.h"
#include "rddoc/net/Processor.h"
#include "rddoc/protocol/proto/AsyncServer.h"
#include "rddoc/protocol/proto/Encoding.h"
#include "rddoc/protocol/proto/Message.h"

namespace rdd {

class PBProcessor : public Processor {
public:
  PBProcessor(PBAsyncServer* server) : server_(server) {}

  virtual ~PBProcessor() {}

  virtual bool decodeData(Event* event) {
    return rdd::proto::decodeData(event->rbuf(), &ibuf_);
  }

  virtual bool encodeData(Event* event) {
    return rdd::proto::encodeData(event->wbuf(), &obuf_);
  }

  virtual bool run() {
    try {
      std::istringstream in(ibuf_);
      int type = proto::readInt(in);
      switch (type) {
        case proto::REQUEST_MSG:
        {
          std::string callId;
          std::shared_ptr<google::protobuf::Message> request;
          const google::protobuf::MethodDescriptor* descriptor = nullptr;
          proto::parseRequestFrom(in, callId, descriptor, request);
          process(callId, descriptor, request);
        }
          break;
        case proto::CANCEL_MSG:
        {
          std::string callId;
          proto::parseCancelFrom(in, callId);
          cancel(callId);
        }
          break;
      }
      return true;
    } catch (std::exception& e) {
      RDDLOG(WARN) << "catch exception: " << e.what();
    } catch (...) {
      RDDLOG(WARN) << "catch unknown exception";
    }
    return false;
  }

private:
  void process(const std::string& callId,
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

  void cancel(const std::string& callId) {
    std::shared_ptr<PBAsyncServer::Handle> handle =
      server_->removeHandle(callId);
    if (handle) {
      PBRpcController controller;
      controller.setCanceled();
      sendResponse(callId, &controller, nullptr);
    } else {
      //
    }
  }

  void finish(std::shared_ptr<PBAsyncServer::Handle> handle) {
    if (handle) {
      if (server_->removeHandle(handle->callId) == handle) {
        sendResponse(handle->callId,
                     handle->controller.get(),
                     handle->response.get());
      } else {
        //
      }
    }
  }

  void sendResponse(const std::string& callId,
                    PBRpcController* controller,
                    google::protobuf::Message* response) {
    std::ostringstream out;
    proto::serializeResponse(callId, *controller, response, out);
    obuf_ = out.str();
  }

  PBAsyncServer* server_;
  std::string ibuf_;
  std::string obuf_;
};

class PBProcessorFactory : public ProcessorFactory {
public:
  PBProcessorFactory(PBAsyncServer* server) : server_(server) {}
  virtual ~PBProcessorFactory() {}

  virtual std::shared_ptr<Processor> create() {
    return std::shared_ptr<Processor>(new PBProcessor(server_));
  }

private:
  PBAsyncServer* server_;
};

}

