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

#include <string>
#include <type_traits>
#include <arpa/inet.h>

#include "accelerator/String.h"
#include "accelerator/Traits.h"
#include "raster/3rd/thrift/protocol/TBinaryProtocol.h"
#include "raster/3rd/thrift/protocol/TJSONProtocol.h"
#include "raster/3rd/thrift/transport/TBufferTransports.h"

namespace rdd {

/*
 * Thrift protocol.
 */
namespace thrift {

namespace {
RDD_CREATE_HAS_MEMBER_FN_TRAITS(has_read_traits, read);
RDD_CREATE_HAS_MEMBER_FN_TRAITS(has_write_traits, write);
}

//////////////////////////////////////////////////////////////////////////////

template <class T>
typename std::enable_if<
  has_write_traits<
    T, uint32_t(::apache::thrift::protocol::TProtocol*) const>::value,
  std::string>::type
serializeToBin(const T& r) {
  boost::shared_ptr<apache::thrift::transport::TMemoryBuffer> buffer(
    new apache::thrift::transport::TMemoryBuffer);
  boost::shared_ptr<apache::thrift::protocol::TBinaryProtocol> protocol(
    new apache::thrift::protocol::TBinaryProtocol(buffer));
  r.write(protocol.get());

  return buffer->getBufferAsString();
}

template <class T>
typename std::enable_if<
  has_read_traits<
    T, uint32_t(::apache::thrift::protocol::TProtocol*)>::value>::type
unserializeFromBin(const std::string& bin, T& r) {
  boost::shared_ptr<apache::thrift::transport::TMemoryBuffer> buffer(
    new apache::thrift::transport::TMemoryBuffer);
  boost::shared_ptr<apache::thrift::protocol::TBinaryProtocol> protocol(
    new apache::thrift::protocol::TBinaryProtocol(buffer));
  buffer->resetBuffer((uint8_t*)bin.data(), bin.size());

  r.read(protocol.get());
}

//////////////////////////////////////////////////////////////////////////////

template <class T>
typename std::enable_if<
  has_write_traits<
    T, uint32_t(::apache::thrift::protocol::TProtocol*) const>::value,
  std::string>::type
serializeToHex(const T& r) {
  boost::shared_ptr<apache::thrift::transport::TMemoryBuffer> buffer(
    new apache::thrift::transport::TMemoryBuffer);
  boost::shared_ptr<apache::thrift::protocol::TBinaryProtocol> protocol(
    new apache::thrift::protocol::TBinaryProtocol(buffer));
  r.write(protocol.get());

  std::string hex;
  hexlify(buffer->getBufferAsString(), hex);
  return hex;
}

template <class T>
typename std::enable_if<
  has_read_traits<
    T, uint32_t(::apache::thrift::protocol::TProtocol*)>::value>::type
unserializeFromHex(const std::string& hex, T& r) {
  std::string bin;
  unhexlify(hex, bin);

  boost::shared_ptr<apache::thrift::transport::TMemoryBuffer> buffer(
    new apache::thrift::transport::TMemoryBuffer);
  boost::shared_ptr<apache::thrift::protocol::TBinaryProtocol> protocol(
    new apache::thrift::protocol::TBinaryProtocol(buffer));
  buffer->resetBuffer((uint8_t*)bin.data(), bin.size());

  r.read(protocol.get());
}

//////////////////////////////////////////////////////////////////////////////

template <class T>
typename std::enable_if<
  has_write_traits<
    T, uint32_t(::apache::thrift::protocol::TProtocol*) const>::value,
  std::string>::type
serializeToJSON(const T& r) {
  boost::shared_ptr<apache::thrift::transport::TMemoryBuffer> buffer(
    new apache::thrift::transport::TMemoryBuffer);
  boost::shared_ptr<apache::thrift::protocol::TJSONProtocol> protocol(
    new apache::thrift::protocol::TJSONProtocol(buffer));
  r.write(protocol.get());

  return buffer->getBufferAsString();
}

template <class T>
typename std::enable_if<
  has_read_traits<
    T, uint32_t(::apache::thrift::protocol::TProtocol*)>::value>::type
unserializeFromJSON(const std::string& json, T& r) {
  boost::shared_ptr<apache::thrift::transport::TMemoryBuffer> buffer(
    new apache::thrift::transport::TMemoryBuffer);
  boost::shared_ptr<apache::thrift::protocol::TJSONProtocol> protocol(
    new apache::thrift::protocol::TJSONProtocol(buffer));
  buffer->resetBuffer((uint8_t*)json.data(), json.size());

  r.read(protocol.get());
}

} // namespace thrift

} // namespace rdd
