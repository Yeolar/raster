/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>

#include "raster/io/Cursor.h"
#include "raster/protocol/proto/IOBufInputStream.h"
#include "raster/protocol/proto/RpcController.h"

namespace rdd {
namespace proto {

enum {
  UNDEFINE_MSG = 0,
  REQUEST_MSG  = 1,
  RESPONSE_MSG = 2,
  CANCEL_MSG   = 3
};

inline char readChar(io::Cursor& in) {
  return in.read<char>();
}

inline void writeChar(char c, IOBufQueue& out) {
  out.append(&c, sizeof(char));
}

inline int readInt(io::Cursor& in) {
  return in.read<int>();
}

inline void writeInt(int i, IOBufQueue& out) {
  out.append(&i, sizeof(int));
}

inline std::string readString(io::Cursor& in) {
  int n = readInt(in);
  return n > 0 ? in.readFixedString(n) : std::string();
}

inline void writeString(const std::string& s, IOBufQueue& out) {
  writeInt(s.length(), out);
  if (s.length() > 0) {
    out.append(s);
  }
}

inline const google::protobuf::MethodDescriptor* readMethodDescriptor(
    io::Cursor& in) {
  return google::protobuf::DescriptorPool::generated_pool()
    ->FindMethodByName(readString(in));
}

inline void writeMethodDescriptor(
    const google::protobuf::MethodDescriptor& method,
    IOBufQueue& out) {
  writeString(method.full_name(), out);
}

inline void readMessage(
    io::Cursor& in,
    std::shared_ptr<google::protobuf::Message>& msg) {
  const google::protobuf::Descriptor* descriptor =
    google::protobuf::DescriptorPool::generated_pool()
      ->FindMessageTypeByName(readString(in));
  msg.reset();
  if (descriptor) {
    msg.reset(google::protobuf::MessageFactory::generated_factory()
              ->GetPrototype(descriptor)->New());
    IOBufInputStream stream(&in);
    if (msg && msg->ParseFromZeroCopyStream(&stream)) {
      return;
    }
    msg.reset();
  }
  throw std::runtime_error("fail to read Message");
}

inline void writeMessage(
    const google::protobuf::Message& msg,
    IOBufQueue& out) {
  writeString(msg.GetDescriptor()->full_name(), out);
  size_t n = msg.ByteSize();
  out.preallocate(n, n);
  msg.SerializeToArray(out.writableTail(), n);
  out.postallocate(n);
}

void parseRequestFrom(
    io::Cursor& in,
    std::string& callId,
    const google::protobuf::MethodDescriptor*& method,
    std::shared_ptr<google::protobuf::Message>& request);

void serializeRequest(
    const std::string& callId,
    const google::protobuf::MethodDescriptor& method,
    const google::protobuf::Message& request,
    IOBufQueue& out);

void parseResponseFrom(
    io::Cursor& in,
    std::string& callId,
    PBRpcController& controller,
    std::shared_ptr<google::protobuf::Message>& response);

void serializeResponse(
    const std::string& callId,
    const PBRpcController& controller,
    const google::protobuf::Message* response,
    IOBufQueue& out);

void parseCancelFrom(
    io::Cursor& in,
    std::string& callId);

void serializeCancel(
    const std::string& callId,
    IOBufQueue& out);

} // namespace proto
} // namespace rdd
