/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <algorithm>
#include <string>
#include <type_traits>
#include <arpa/inet.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/protocol/TJSONProtocol.h>
#include <thrift/transport/TBufferTransports.h>
#include "raster/util/String.h"
#include "raster/util/Traits.h"

namespace rdd {

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
  !has_write_traits<
    T, uint32_t(::apache::thrift::protocol::TProtocol*) const>::value,
  std::string>::type
serializeToBin(const T& r) {
  return "non-thrift struct";
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

template <class T>
typename std::enable_if<
  !has_read_traits<
    T, uint32_t(::apache::thrift::protocol::TProtocol*)>::value>::type
unserializeFromBin(const std::string& bin, T& r) {
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
  !has_write_traits<
    T, uint32_t(::apache::thrift::protocol::TProtocol*) const>::value,
  std::string>::type
serializeToHex(const T& r) {
  return "non-thrift struct";
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

template <class T>
typename std::enable_if<
  !has_read_traits<
    T, uint32_t(::apache::thrift::protocol::TProtocol*)>::value>::type
unserializeFromHex(const std::string& hex, T& r) {
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
  !has_write_traits<
    T, uint32_t(::apache::thrift::protocol::TProtocol*) const>::value,
  std::string>::type
serializeToJSON(const T& r) {
  return "non-thrift struct";
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

template <class T>
typename std::enable_if<
  !has_read_traits<
    T, uint32_t(::apache::thrift::protocol::TProtocol*)>::value>::type
unserializeFromJSON(const std::string& json, T& r) {
}

} // namespace thrift

} // namespace rdd
