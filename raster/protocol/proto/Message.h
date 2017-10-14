/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <istream>
#include <memory>
#include <ostream>
#include <string>
#include <google/protobuf/service.h>
#include "raster/io/Cursor.h"
#include "raster/protocol/proto/RpcController.h"

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
    io::Appender& out);

void parseRequestFrom(
    io::RWPrivateCursor& in,
    std::string& callId,
    const google::protobuf::MethodDescriptor*& method,
    std::shared_ptr<google::protobuf::Message>& request);

void serializeResponse(
    const std::string& callId,
    const PBRpcController& controller,
    const google::protobuf::Message* response,
    io::Appender& out);

void parseResponseFrom(
    io::RWPrivateCursor& in,
    std::string& callId,
    PBRpcController& controller,
    std::shared_ptr<google::protobuf::Message>& response);

void serializeCancel(
    const std::string& callId,
    io::Appender& out);

void parseCancelFrom(
    io::RWPrivateCursor& in,
    std::string& callId);

} // namespace proto
} // namespace rdd
