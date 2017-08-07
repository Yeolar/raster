/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <string>
#include <ostream>
#include <streambuf>

namespace rdd {

class FixedStreamBuf : public std::streambuf {
public:
  FixedStreamBuf(char* buf, size_t len) {
    setp(buf, buf + len);
  }

  FixedStreamBuf(const char* buf, const char* next, const char* end) {
    setg((char*)buf, (char*)next, (char*)end);
  }

  std::string str() { return std::string(pbase(), pptr()); }
};

class FixedOstream : private virtual FixedStreamBuf, public std::ostream {
public:
  typedef FixedStreamBuf StreamBuf;

  FixedOstream(char* buf, size_t len)
    : FixedStreamBuf(buf, len),
      std::ostream(static_cast<StreamBuf*>(this)) {}

  char* output() { return StreamBuf::pbase(); }
  char* output_ptr() { return StreamBuf::pptr(); }
  char* output_end() { return StreamBuf::epptr(); }

  void reset() { StreamBuf::setp(output(), output_end()); }

  std::string str() { return StreamBuf::str(); }
};

} // namespace rdd
