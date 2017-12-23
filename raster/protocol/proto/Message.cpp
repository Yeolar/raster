/*
* Copyright (C) 2017, Yeolar
*/

#include "raster/protocol/proto/Message.h"

namespace rdd {
namespace proto {

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
