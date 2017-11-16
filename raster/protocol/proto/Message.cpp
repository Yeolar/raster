/*
* Copyright (C) 2017, Yeolar
*/

#include "raster/protocol/proto/Encoding.h"
#include "raster/protocol/proto/Message.h"

namespace rdd {
namespace proto {

void serializeRequest(
    const std::string& callId,
    const google::protobuf::MethodDescriptor& method,
    const google::protobuf::Message& request,
    io::Appender& out) {
  proto::writeInt(REQUEST_MSG, out);
  proto::writeString(callId, out);
  proto::writeMethodDescriptor(method, out);
  proto::writeMessage(request, out);
}

void parseRequestFrom(
    io::RWPrivateCursor& in,
    std::string& callId,
    const google::protobuf::MethodDescriptor*& method,
    std::shared_ptr<google::protobuf::Message>& request) {
  callId = proto::readString(in);
  method = proto::readMethodDescriptor(in);
  request.reset();
  proto::readMessage(in, request);
}

void serializeResponse(
    const std::string& callId,
    const PBRpcController& controller,
    const google::protobuf::Message* response,
    io::Appender& out) {
  proto::writeInt(RESPONSE_MSG, out);
  proto::writeString(callId, out);
  controller.serializeTo(out);
  if (response) {
    out.write<char>('Y');
    proto::writeMessage(*response, out);
  } else {
    out.write<char>('N');
  }
}

void parseResponseFrom(
    io::RWPrivateCursor& in,
    std::string& callId,
    PBRpcController& controller,
    std::shared_ptr<google::protobuf::Message>& response) {
  callId = proto::readString(in);
  controller.parseFrom(in);
  char c = in.read<char>();
  if (c == 'Y') {
    response.reset();
    proto::readMessage(in, response);
  }
}

void serializeCancel(const std::string& callId, io::Appender& out) {
  proto::writeInt(CANCEL_MSG, out);
  proto::writeString(callId, out);
}

void parseCancelFrom(io::RWPrivateCursor& in, std::string& callId) {
  callId = proto::readString(in);
}

} // namespace proto
} // namespace rdd
