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
#include "raster/net/Protocol.h"
#include "raster/io/Cursor.h"
#include "raster/io/TypedIOBuf.h"

namespace rdd {
namespace proto {

// buf -> ibuf
inline bool decodeData(std::unique_ptr<IOBuf>& buf,
                       std::unique_ptr<IOBuf>& ibuf) {
  io::Cursor cursor(buf.get());
  uint32_t header = cursor.read<uint32_t>();
  RDDLOG(V3) << "decode proto size: " << ntohl(header);
  buf->trimStart(sizeof(uint32_t));
  ibuf.swap(buf);
  return true;
}

// obuf -> buf
inline bool encodeData(std::unique_ptr<IOBuf>& buf,
                       std::unique_ptr<IOBuf>& obuf) {
  uint32_t n = obuf->computeChainDataLength();
  RDDLOG(V3) << "encode proto size: " << n;
  TypedIOBuf<uint32_t>(buf.get()).push(htonl(n));
  std::unique_ptr<IOBuf> tmp;
  tmp.swap(obuf);
  buf->appendChain(std::move(tmp));
  return true;
}

/*
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
*/
inline char readChar(io::RWPrivateCursor& in) {
  return in.read<char>();
}

/*
inline void writeChar(char c, std::ostream& out) {
  out.put(c);
}
*/
inline void writeChar(char c, io::Appender& out) {
  out.write<char>(c);
}

/*
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
*/
inline int readInt(io::RWPrivateCursor& in) {
  return in.read<int>();
}

/*
inline void writeInt(int i, std::ostream& out) {
  char buf[4];
  buf[0] = (char)((i >> 24) & 0xff);
  buf[1] = (char)((i >> 16) & 0xff);
  buf[2] = (char)((i >>  8) & 0xff);
  buf[3] = (char)((i >>  0) & 0xff);
  out.write(buf, sizeof(buf));
}
*/
inline void writeInt(int i, io::Appender& out) {
  out.write<int>(i);
}

/*
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
*/
inline std::string readString(io::RWPrivateCursor& in) {
  int n = readInt(in);
  if (n > 0) {
    return in.readFixedString(n);
  }
  return std::string();
}

/*
inline void writeString(const std::string& s, std::ostream& out) {
  writeInt(s.length(), out);
  if (s.length() > 0) {
    out.write(s.data(), s.length());
  }
}
*/
inline void writeString(const std::string& s, io::Appender& out) {
  writeInt(s.length(), out);
  if (s.length() > 0) {
    out.push(range(s));
  }
}

inline const google::protobuf::MethodDescriptor* readMethodDescriptor(
    io::RWPrivateCursor& in) {
  return google::protobuf::DescriptorPool::generated_pool()
    ->FindMethodByName(readString(in));
}

inline void writeMethodDescriptor(
    const google::protobuf::MethodDescriptor& method, io::Appender& out) {
  writeString(method.full_name(), out);
}

inline void readMessage(
    io::RWPrivateCursor& in, std::shared_ptr<google::protobuf::Message>& msg) {
  const google::protobuf::Descriptor* descriptor =
    google::protobuf::DescriptorPool::generated_pool()
      ->FindMessageTypeByName(readString(in));
  msg.reset();
  if (descriptor) {
    msg.reset(google::protobuf::MessageFactory::generated_factory()
              ->GetPrototype(descriptor)->New());
    size_t n = in.totalLength();
    in.gather(n);
    if (msg && msg->ParsePartialFromArray(in.data(), n)) {
      return;
    }
    msg.reset();
  }
  throw std::runtime_error("fail to read Message");
}

inline void writeMessage(
    const google::protobuf::Message& msg, io::Appender& out) {
  writeString(msg.GetDescriptor()->full_name(), out);
  size_t n = msg.ByteSize();
  out.ensure(n);
  msg.SerializeToArray(out.writableData(), n);
  out.append(n);
}

} // namespace proto
} // namespace rdd
