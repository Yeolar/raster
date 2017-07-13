/*
* Copyright (C) 2017, Yeolar
*/

#include "rddoc/protocol/proto/Message.h"
#include "rddoc/protocol/proto/Encoding.h"

namespace rdd {
namespace proto {

void serializeRequest(
    const std::string& callId,
    const google::protobuf::MethodDescriptor& method,
    const google::protobuf::Message& request,
    std::ostream& out) {
  proto::writeInt(REQUEST_MSG, out);
  proto::writeString(callId, out);
  proto::writeMethodDescriptor(method, out);
  proto::writeMessage(request, out);
}

void parseRequestFrom(
    std::istream& in,
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
    std::ostream& out) {
  proto::writeInt(RESPONSE_MSG, out);
  proto::writeString(callId, out);
  controller.serializeTo(out);
  if (response) {
    out.put('Y');
    proto::writeMessage(*response, out);
  } else {
    out.put('N');
  }
}

void parseResponseFrom(
    std::istream& in,
    std::string& callId,
    PBRpcController& controller,
    std::shared_ptr<google::protobuf::Message>& response) {
  callId = proto::readString(in);
  controller.parseFrom(in);
  char c = in.get();
  if (c == 'Y') {
    response.reset();
    proto::readMessage(in, response);
  }
}

void serializeCancel(const std::string& callId, std::ostream& out) {
  proto::writeInt(CANCEL_MSG, out);
  proto::writeString(callId, out);
}

void parseCancelFrom(std::istream& in, std::string& callId) {
  callId = proto::readString(in);
}

}
}
