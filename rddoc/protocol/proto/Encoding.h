/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <istream>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <arpa/inet.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#include "rddoc/net/Protocol.h"
#include "rddoc/io/Cursor.h"
#include "rddoc/io/TypedIOBuf.h"

namespace rdd {
namespace proto {

// buf -> ibuf
inline bool decodeData(IOBuf* buf, std::string* ibuf) {
  auto range = buf->coalesce();
  RDDLOG_ON(V4) {
    std::string hex;
    hexlify(range, hex);
    RDDLOG(V4) << "decode proto data: " << hex;
  }
  uint32_t header = *TypedIOBuf<uint32_t>(buf).data();
  RDDLOG(V3) << "decode proto size: " << ntohl(header);
  range.advance(sizeof(uint32_t));
  ibuf->assign((const char*)range.data(), range.size());
  return true;
}

// obuf -> buf
inline bool encodeData(IOBuf* buf, std::string* obuf) {
  uint8_t* p = (uint8_t*)&(*obuf)[0];
  uint32_t n = obuf->size();
  RDDLOG(V3) << "encode proto size: " << n;
  TypedIOBuf<uint32_t>(buf).push(htonl(n));
  rdd::io::Appender appender(buf, Protocol::CHUNK_SIZE);
  appender.pushAtMost(p, n);
  RDDLOG_ON(V4) {
    auto range = buf->coalesce();
    std::string hex;
    hexlify(range, hex);
    RDDLOG(V4) << "encode proto data: " << hex;
  }
  return true;
}

inline char readChar(const std::string& s, size_t& offset) {
  if (s.length() < offset + sizeof(char)) {
    throw std::runtime_error("fail to read a char");
  }
  char c = s[offset];
  offset += sizeof(char);
  return c;
}

inline char readChar(std::istream& in) {
  char c = in.get();
  if (in.gcount() != sizeof(char)) {
    throw std::runtime_error("fail to read a char");
  }
  return c;
}

inline void writeChar(char c, std::ostream& out) {
  out.put(c);
}

inline int readInt(const std::string& s, size_t& offset) {
  if (s.length() < offset + 4) {
    throw std::runtime_error("fail to read int from string");
  }
  int i = ((s[offset]   & 0xff) << 24) |
          ((s[offset+1] & 0xff) << 16) |
          ((s[offset+2] & 0xff) <<  8) |
          ((s[offset+3] & 0xff) <<  0);
  offset += 4;
  return i;
}

inline int readInt(std::istream& in) {
  char buf[4];
  in.read(buf, sizeof(buf));
  if (in.gcount() == 4) {
    return ((buf[0] & 0xff) << 24) |
           ((buf[1] & 0xff) << 16) |
           ((buf[2] & 0xff) <<  8) |
           ((buf[3] & 0xff) <<  0);
  }
  throw std::runtime_error("fail to read int from stream");
}

inline void writeInt(int i, std::ostream& out) {
  char buf[4];
  buf[0] = (char)((i >> 24) & 0xff);
  buf[1] = (char)((i >> 16) & 0xff);
  buf[2] = (char)((i >>  8) & 0xff);
  buf[3] = (char)((i >>  0) & 0xff);
  out.write(buf, sizeof(buf));
}

inline std::string readString(std::istream& in) {
  int n = readInt(in);
  if (n > 0) {
    std::string s;
    s.resize(n);
    in.read(&s[0], n);
    if (in.gcount() == n) {
      return s;
    } else {
      throw std::runtime_error("fail to read a string from input stream");
    }
  }
  return std::string();
}

inline void writeString(const std::string& s, std::ostream& out) {
  writeInt(s.length(), out);
  if (s.length() > 0) {
    out.write(s.data(), s.length());
  }
}

inline const google::protobuf::MethodDescriptor* readMethodDescriptor(
    std::istream& in) {
  return google::protobuf::DescriptorPool::generated_pool()
    ->FindMethodByName(readString(in));
}

inline void writeMethodDescriptor(
    const google::protobuf::MethodDescriptor& method, std::ostream& out) {
  writeString(method.full_name(), out);
}

inline void readMessage(
    std::istream& in, std::shared_ptr<google::protobuf::Message>& msg) {
  const google::protobuf::Descriptor* descriptor =
    google::protobuf::DescriptorPool::generated_pool()
      ->FindMessageTypeByName(readString(in));
  msg.reset();
  if (descriptor) {
    msg.reset(google::protobuf::MessageFactory::generated_factory()
              ->GetPrototype(descriptor)->New());
    if (msg && msg->ParsePartialFromIstream(&in)) {
      return;
    }
    msg.reset();
  }
  throw std::runtime_error("fail to read Message");
}

inline std::shared_ptr<google::protobuf::Message> readMessage(
    std::istream& in) {
  std::shared_ptr<google::protobuf::Message> msg;
  readMessage(in, msg);
  return msg;
}

inline void writeMessage(
    const google::protobuf::Message& msg, std::ostream& out) {
  writeString(msg.GetDescriptor()->full_name(), out);
  msg.SerializeToOstream(&out);
}

} // namespace proto
} // namespace rdd
