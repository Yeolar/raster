/*
* Copyright (C) 2017, Yeolar
*/

#include "raster/protocol/proto/Message.h"

#include "raster/protocol/proto/IOBufInputStream.h"

namespace rdd {
namespace proto {

char readChar(io::Cursor& in) {
  return in.read<char>();
}

void writeChar(char c, IOBufQueue& out) {
  out.append(&c, sizeof(char));
}

int readInt(io::Cursor& in) {
  return in.read<int>();
}

void writeInt(int i, IOBufQueue& out) {
  out.append(&i, sizeof(int));
}

std::string readString(io::Cursor& in) {
  int n = readInt(in);
  return n > 0 ? in.readFixedString(n) : std::string();
}

void writeString(const std::string& s, IOBufQueue& out) {
  writeInt(s.length(), out);
  if (s.length() > 0) {
    out.append(s);
  }
}

const google::protobuf::MethodDescriptor* readMethodDescriptor(
    io::Cursor& in) {
  return google::protobuf::DescriptorPool::generated_pool()
    ->FindMethodByName(readString(in));
}

void writeMethodDescriptor(
    const google::protobuf::MethodDescriptor& method,
    IOBufQueue& out) {
  writeString(method.full_name(), out);
}

void readMessage(
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

void writeMessage(
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
    std::shared_ptr<google::protobuf::Message>& request) {
  callId = proto::readString(in);
  method = proto::readMethodDescriptor(in);
  request.reset();
  proto::readMessage(in, request);
}

void serializeRequest(
    const std::string& callId,
    const google::protobuf::MethodDescriptor& method,
    const google::protobuf::Message& request,
    IOBufQueue& out) {
  proto::writeInt(REQUEST_MSG, out);
  proto::writeString(callId, out);
  proto::writeMethodDescriptor(method, out);
  proto::writeMessage(request, out);
}

void parseResponseFrom(
    io::Cursor& in,
    std::string& callId,
    PBRpcController& controller,
    std::shared_ptr<google::protobuf::Message>& response) {
  callId = proto::readString(in);
  controller.parseFrom(in);
  char c = proto::readChar(in);
  if (c == 'Y') {
    response.reset();
    proto::readMessage(in, response);
  }
}

void serializeResponse(
    const std::string& callId,
    const PBRpcController& controller,
    const google::protobuf::Message* response,
    IOBufQueue& out) {
  proto::writeInt(RESPONSE_MSG, out);
  proto::writeString(callId, out);
  controller.serializeTo(out);
  if (response) {
    proto::writeChar('Y', out);
    proto::writeMessage(*response, out);
  } else {
    proto::writeChar('N', out);
  }
}

void parseCancelFrom(
    io::Cursor& in,
    std::string& callId) {
  callId = proto::readString(in);
}

void serializeCancel(
    const std::string& callId,
    IOBufQueue& out) {
  proto::writeInt(CANCEL_MSG, out);
  proto::writeString(callId, out);
}

} // namespace proto
} // namespace rdd
