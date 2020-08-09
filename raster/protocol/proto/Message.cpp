/*
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "raster/protocol/proto/Message.h"

#include "raster/protocol/proto/IOBufInputStream.h"

namespace raster {
namespace proto {

char readChar(acc::io::Cursor& in) {
  return in.read<char>();
}

void writeChar(char c, acc::IOBufQueue& out) {
  out.append(&c, sizeof(char));
}

int readInt(acc::io::Cursor& in) {
  return in.read<int>();
}

void writeInt(int i, acc::IOBufQueue& out) {
  out.append(&i, sizeof(int));
}

std::string readString(acc::io::Cursor& in) {
  int n = readInt(in);
  return n > 0 ? in.readFixedString(n) : std::string();
}

void writeString(const std::string& s, acc::IOBufQueue& out) {
  writeInt(s.length(), out);
  if (s.length() > 0) {
    out.append(s);
  }
}

const google::protobuf::MethodDescriptor* readMethodDescriptor(
    acc::io::Cursor& in) {
  return google::protobuf::DescriptorPool::generated_pool()
    ->FindMethodByName(readString(in));
}

void writeMethodDescriptor(
    const google::protobuf::MethodDescriptor& method,
    acc::IOBufQueue& out) {
  writeString(method.full_name(), out);
}

void readMessage(
    acc::io::Cursor& in,
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
    acc::IOBufQueue& out) {
  writeString(msg.GetDescriptor()->full_name(), out);
  size_t n = msg.ByteSize();
  out.preallocate(n, n);
  msg.SerializeToArray(out.writableTail(), n);
  out.postallocate(n);
}

void parseRequestFrom(
    acc::io::Cursor& in,
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
    acc::IOBufQueue& out) {
  proto::writeInt(REQUEST_MSG, out);
  proto::writeString(callId, out);
  proto::writeMethodDescriptor(method, out);
  proto::writeMessage(request, out);
}

void parseResponseFrom(
    acc::io::Cursor& in,
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
    acc::IOBufQueue& out) {
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
    acc::io::Cursor& in,
    std::string& callId) {
  callId = proto::readString(in);
}

void serializeCancel(
    const std::string& callId,
    acc::IOBufQueue& out) {
  proto::writeInt(CANCEL_MSG, out);
  proto::writeString(callId, out);
}

} // namespace proto
} // namespace raster
