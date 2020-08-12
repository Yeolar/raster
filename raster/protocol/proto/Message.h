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

#pragma once

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>

#include <accelerator/io/Cursor.h>

#include "raster/protocol/proto/RpcController.h"

namespace raster {
namespace proto {

enum {
  UNDEFINE_MSG = 0,
  REQUEST_MSG  = 1,
  RESPONSE_MSG = 2,
  CANCEL_MSG   = 3
};

char readChar(acc::io::Cursor& in);

void writeChar(char c, acc::IOBufQueue& out);

int readInt(acc::io::Cursor& in);

void writeInt(int i, acc::IOBufQueue& out);

std::string readString(acc::io::Cursor& in);

void writeString(const std::string& s, acc::IOBufQueue& out);

const google::protobuf::MethodDescriptor* readMethodDescriptor(
    acc::io::Cursor& in);

void writeMethodDescriptor(
    const google::protobuf::MethodDescriptor& method,
    acc::IOBufQueue& out);

void readMessage(
    acc::io::Cursor& in,
    std::shared_ptr<google::protobuf::Message>& msg);

void writeMessage(
    const google::protobuf::Message& msg,
    acc::IOBufQueue& out);

void parseRequestFrom(
    acc::io::Cursor& in,
    std::string& callId,
    const google::protobuf::MethodDescriptor*& method,
    std::shared_ptr<google::protobuf::Message>& request);

void serializeRequest(
    const std::string& callId,
    const google::protobuf::MethodDescriptor& method,
    const google::protobuf::Message& request,
    acc::IOBufQueue& out);

void parseResponseFrom(
    acc::io::Cursor& in,
    std::string& callId,
    PBRpcController& controller,
    std::shared_ptr<google::protobuf::Message>& response);

void serializeResponse(
    const std::string& callId,
    const PBRpcController& controller,
    const google::protobuf::Message* response,
    acc::IOBufQueue& out);

void parseCancelFrom(
    acc::io::Cursor& in,
    std::string& callId);

void serializeCancel(
    const std::string& callId,
    acc::IOBufQueue& out);

} // namespace proto
} // namespace raster
