/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <istream>
#include <memory>
#include <ostream>
#include <string>
#include <google/protobuf/service.h>
#include "rddoc/protocol/proto/RpcController.h"

namespace rdd {
namespace proto {

enum {
  UNDEFINE_MSG = 0,
  REQUEST_MSG  = 1,
  RESPONSE_MSG = 2,
  CANCEL_MSG   = 3
};

void serializeRequest(
    const std::string& callId,
    const google::protobuf::MethodDescriptor& method,
    const google::protobuf::Message& request,
    std::ostream& out);

void parseRequestFrom(
    std::istream& in,
    std::string& callId,
    const google::protobuf::MethodDescriptor*& method,
    std::shared_ptr<google::protobuf::Message>& request);

void serializeResponse(
    const std::string& callId,
    const PBRpcController& controller,
    const google::protobuf::Message* response,
    std::ostream& out );

void parseResponseFrom(
    std::istream& in,
    std::string& callId,
    PBRpcController& controller,
    std::shared_ptr<google::protobuf::Message>& response);

void serializeCancel(const std::string& callId, std::ostream& out);

void parseCancelFrom(std::istream& in, std::string& callId);

}
}
