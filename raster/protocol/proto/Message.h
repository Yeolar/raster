/*
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
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

char readChar(io::Cursor& in);

void writeChar(char c, IOBufQueue& out);

int readInt(io::Cursor& in);

void writeInt(int i, IOBufQueue& out);

std::string readString(io::Cursor& in);

void writeString(const std::string& s, IOBufQueue& out);

const google::protobuf::MethodDescriptor* readMethodDescriptor(
    io::Cursor& in);

void writeMethodDescriptor(
    const google::protobuf::MethodDescriptor& method,
    IOBufQueue& out);

void readMessage(
    io::Cursor& in,
    std::shared_ptr<google::protobuf::Message>& msg);

void writeMessage(
    const google::protobuf::Message& msg,
    IOBufQueue& out);

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
