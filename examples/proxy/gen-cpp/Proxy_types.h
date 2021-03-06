/**
 * Autogenerated by Thrift Compiler (0.9.1)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef Proxy_TYPES_H
#define Proxy_TYPES_H

#include <thrift/Thrift.h>
#include <thrift/TApplicationException.h>
#include <thrift/protocol/TProtocol.h>
#include <thrift/transport/TTransport.h>

#include <thrift/cxxfunctional.h>


namespace rdd { namespace proxy {

struct ResultCode {
  enum type {
    OK = 0,
    E_SOURCE__UNTRUSTED = 1001,
    E_BACKEND_FAILURE = 1002
  };
};

extern const std::map<int, const char*> _ResultCode_VALUES_TO_NAMES;

typedef struct _Query__isset {
  _Query__isset() : query(false), forward(false) {}
  bool query;
  bool forward;
} _Query__isset;

class Query {
 public:

  static const char* ascii_fingerprint; // = "4BF81DD46A7371532E49811022D58D36";
  static const uint8_t binary_fingerprint[16]; // = {0x4B,0xF8,0x1D,0xD4,0x6A,0x73,0x71,0x53,0x2E,0x49,0x81,0x10,0x22,0xD5,0x8D,0x36};

  Query() : traceid(), query(), forward() {
  }

  virtual ~Query() throw() {}

  std::string traceid;
  std::string query;
  std::string forward;

  _Query__isset __isset;

  void __set_traceid(const std::string& val) {
    traceid = val;
  }

  void __set_query(const std::string& val) {
    query = val;
    __isset.query = true;
  }

  void __set_forward(const std::string& val) {
    forward = val;
    __isset.forward = true;
  }

  bool operator == (const Query & rhs) const
  {
    if (!(traceid == rhs.traceid))
      return false;
    if (__isset.query != rhs.__isset.query)
      return false;
    else if (__isset.query && !(query == rhs.query))
      return false;
    if (__isset.forward != rhs.__isset.forward)
      return false;
    else if (__isset.forward && !(forward == rhs.forward))
      return false;
    return true;
  }
  bool operator != (const Query &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const Query & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

void swap(Query &a, Query &b);

typedef struct _Result__isset {
  _Result__isset() : code(false) {}
  bool code;
} _Result__isset;

class Result {
 public:

  static const char* ascii_fingerprint; // = "214512252A0944207CAC77897767CC5E";
  static const uint8_t binary_fingerprint[16]; // = {0x21,0x45,0x12,0x25,0x2A,0x09,0x44,0x20,0x7C,0xAC,0x77,0x89,0x77,0x67,0xCC,0x5E};

  Result() : traceid(), code((ResultCode::type)0) {
  }

  virtual ~Result() throw() {}

  std::string traceid;
  ResultCode::type code;

  _Result__isset __isset;

  void __set_traceid(const std::string& val) {
    traceid = val;
  }

  void __set_code(const ResultCode::type val) {
    code = val;
    __isset.code = true;
  }

  bool operator == (const Result & rhs) const
  {
    if (!(traceid == rhs.traceid))
      return false;
    if (__isset.code != rhs.__isset.code)
      return false;
    else if (__isset.code && !(code == rhs.code))
      return false;
    return true;
  }
  bool operator != (const Result &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const Result & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

void swap(Result &a, Result &b);

}} // namespace

#endif
