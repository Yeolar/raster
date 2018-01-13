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

#include <ostream>
#include <streambuf>
#include <string>

namespace rdd {

class FixedStreamBuf : public std::streambuf {
 public:
  FixedStreamBuf(char* buf, size_t len) {
    setp(buf, buf + len);
  }

  FixedStreamBuf(const char* buf, const char* next, const char* end) {
    setg((char*)buf, (char*)next, (char*)end);
  }

  std::string str() {
    return std::string(pbase(), pptr());
  }
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

  void reset() {
    StreamBuf::setp(output(), output_end());
  }

  void advance(int n) {
    StreamBuf::pbump(n);
  }

  std::string str() {
    return StreamBuf::str();
  }
};

} // namespace rdd
