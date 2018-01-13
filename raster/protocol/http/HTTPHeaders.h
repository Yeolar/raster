/*
 * Copyright (c) 2015, Facebook, Inc.
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

#include <bitset>
#include <cstring>
#include <string>

#include "raster/protocol/http/HTTPCommonHeaders.h"
#include "raster/util/Range.h"

namespace rdd {

extern const std::string empty_string;

/**
 * Byte counters related to the HTTP headers.
 */
struct HTTPHeaderSize {
  size_t compressed{0};   // 0 for compression not supported
  size_t uncompressed{0};
};

/**
 * A collection of HTTP headers.
 *
 * Headers are stored as Name/Value pairs, in the order they are received on
 * the wire. We hash the names of all common HTTP headers (using a static
 * perfect hash function generated using gperf from HTTPCommonHeaders.gperf)
 * into 1-byte hashes (we call them "codes") and only store these. We search
 * them using memchr, which has an x86_64 assembly implementation with
 * complexity O(n/16) ;)
 *
 * Instead of creating strings with header names, we point to a static array
 * of strings in HTTPCommonHeaders. If the header name is not in our set of
 * common header names (this is considered unlikely, because we intend this set
 * to be very complete), then we create a new string with its name (we own that
 * pointer then). For such headers, we store the code HTTP_HEADER_OTHER.
 *
 * The code HTTP_HEADER_NONE signifies a header that has been removed.
 *
 * Most methods which take a header name have two versions: one accepting
 * a string, and one accepting a code. It is recommended to use the latter
 * if possible, as in:
 *     headers.add(HTTP_HEADER_LOCATION, location);
 * rather than:
 *     headers.add("Location", location);
 */
class HTTPHeaders {
 public:
  /*
   * separator used to concatenate multiple values of the same header
   * check out sections 4.2 and 14.45 from rfc2616
   */
  static const std::string COMBINE_SEPARATOR;

  HTTPHeaders();
  ~HTTPHeaders();
  HTTPHeaders (const HTTPHeaders&);
  HTTPHeaders& operator= (const HTTPHeaders&);
  HTTPHeaders (HTTPHeaders&&) noexcept;
  HTTPHeaders& operator= (HTTPHeaders&&);

  void parse(StringPiece sp);

  void add(StringPiece name, StringPiece value);
  template <typename T> // T = string
  void add(StringPiece name, T&& value);
  template <typename T> // T = string
  void add(HTTPHeaderCode code, T&& value);

  void set(StringPiece name, const std::string& value) {
    remove(name);
    add(name, value);
  }
  void set(HTTPHeaderCode code, const std::string& value) {
    remove(code);
    add(code, value);
  }

  bool exists(StringPiece name) const;
  bool exists(HTTPHeaderCode code) const;

  template <typename T>
  std::string combine(const T& header,
                      const std::string& separator=COMBINE_SEPARATOR) const;

  /**
   * Process the list of all headers, in the order that they were seen:
   * for each header:value pair, the function/functor/lambda-expression
   * given as the second parameter will be executed. It should take two
   * const string & parameters and return void. Example use:
   *     hdrs.forEach([&] (const string& header, const string& val) {
   *       std::cout << header << ": " << val;
   *     });
   */
  template <typename LAMBDA> // (const string &, const string &) -> void
  inline void forEach(LAMBDA func) const;

  /**
   * Process the list of all headers, in the order that they were seen:
   * for each header:value pair, the function/functor/lambda-expression
   * given as the second parameter will be executed. It should take one
   * HTTPHeaderCode (code) parameter, two const string & parameters and
   * return void. Example use:
   *     hdrs.forEachWithCode([&] (HTTPHeaderCode code,
   *                               const string& header,
   *                               const string& val) {
   *       std::cout << header << "(" << code << "): " << val;
   *     });
   */
  template <typename LAMBDA>
  inline void forEachWithCode(LAMBDA func) const;

  /**
   * Process the list of all headers, in the order that they were seen:
   * for each header:value pair, the function/functor/lambda-expression
   * given as the parameter will be executed to determine whether the
   * header should be removed. Example use:
   *
   *     hdrs.removeByPredicate([&] (HTTPHeaderCode code,
   *                                 const string& header,
   *                                 const string& val) {
   *       return boost::regex_match(header, "^X-Fb-.*");
   *     });
   *
   * return true only if one or more headers are removed.
   */
  template <typename LAMBDA> // (const string &, const string &) -> bool
  inline bool removeByPredicate(LAMBDA func);

  /**
   * Returns the value of the header if it's found in the message and is the
   * only value under the given name.
   */
  template <typename T> // either uint8_t or string
  const std::string & getSingleOrEmpty(const T& nameOrCode) const;
  template <typename T> // either uint8_t or string
  std::string getSingle(const T& nameOrCode, const std::string& dflt="") const;

  size_t getNumberOfValues(HTTPHeaderCode code) const;
  size_t getNumberOfValues(StringPiece name) const;

  /**
   * Process the ordered list of values for the given header name:
   * for each value, the function/functor/lambda-expression given as the second
   * parameter will be executed. It should take one const string & parameter
   * and return bool (false to keep processing, true to stop it). Example use:
   *     hdrs.forEachValueOfHeader("someheader", [&] (const string& val) {
   *       std::cout << val;
   *       return false;
   *     });
   * This method returns true if processing was stopped (by func returning
   * true), and false otherwise.
   */
  template <typename LAMBDA> // const string & -> bool
  inline bool forEachValueOfHeader(StringPiece name, LAMBDA func) const;
  template <typename LAMBDA> // const string & -> bool
  inline bool forEachValueOfHeader(HTTPHeaderCode code, LAMBDA func) const;

  bool remove(StringPiece name);
  bool remove(HTTPHeaderCode code);

  void removeAll();

  size_t size() const;

  void copyTo(HTTPHeaders& hdrs) const;

  /**
   * 304 responses should not contain entity headers
   */
  void clearHeadersFor304();

  /**
   * Remove per-hop-headers and headers named in the Connection header
   * and place the value in strippedHeaders
   */
  void stripPerHopHeaders(HTTPHeaders& strippedHeaders);

  static std::bitset<256>& entityHeaderCodes();
  static std::bitset<256>& perHopHeaderCodes();

 private:
  std::vector<HTTPHeaderCode> codes_;
  std::vector<const std::string *> headerNames_;
  std::vector<std::string> headerValues_;
  size_t deletedCount_;

  static const size_t kInitialVectorReserve = 16;

  bool transferHeaderIfPresent(StringPiece name, HTTPHeaders& dest);

  static void initGlobals() __attribute__ ((__constructor__));

  void disposeOfHeaderNames();
};

template <typename T> // T = string
void HTTPHeaders::add(StringPiece name, T&& value) {
  assert(name.size());
  const HTTPHeaderCode code = HTTPCommonHeaders::hash(name.data(), name.size());
  codes_.push_back(code);
  headerNames_.push_back((code == HTTP_HEADER_OTHER)
      ? new std::string(name.data(), name.size())
      : HTTPCommonHeaders::getPointerToHeaderName(code));
  headerValues_.emplace_back(std::forward<T>(value));
}

template <typename T> // T = string
void HTTPHeaders::add(HTTPHeaderCode code, T&& value) {
  codes_.push_back(code);
  headerNames_.push_back(HTTPCommonHeaders::getPointerToHeaderName(code));
  headerValues_.emplace_back(std::forward<T>(value));
}

// iterate over the positions (in vector) of all headers with given code
#define ITERATE_OVER_CODES(Code, Block) { \
  const HTTPHeaderCode* ptr = codes_.data(); \
  while(true) { \
    ptr = (HTTPHeaderCode*) memchr((void*)ptr, (Code), \
                            codes_.size() - (ptr - codes_.data())); \
    if (ptr == nullptr) break; \
    const size_t pos = ptr - codes_.data(); \
    {Block} \
    ptr++; \
  } \
}

// iterate over the positions of all headers with given name
#define ITERATE_OVER_STRINGS(String, Block) \
    ITERATE_OVER_CODES(HTTP_HEADER_OTHER, { \
  if (caseInsensitiveEqual((String), *headerNames_[pos])) { \
    {Block} \
  } \
})

template <typename LAMBDA> // (const string &, const string &) -> void
void HTTPHeaders::forEach(LAMBDA func) const {
  for (size_t i = 0; i < codes_.size(); ++i) {
    if (codes_[i] != HTTP_HEADER_NONE) {
      func(*headerNames_[i], headerValues_[i]);
    }
  }
}

template <typename LAMBDA>
void HTTPHeaders::forEachWithCode(LAMBDA func) const {
  for (size_t i = 0; i < codes_.size(); ++i) {
    if (codes_[i] != HTTP_HEADER_NONE) {
      func(codes_[i], *headerNames_[i], headerValues_[i]);
    }
  }
}

template <typename LAMBDA> // const string & -> bool
bool HTTPHeaders::forEachValueOfHeader(StringPiece name, LAMBDA func) const {
  const HTTPHeaderCode code = HTTPCommonHeaders::hash(name.data(), name.size());
  if (code != HTTP_HEADER_OTHER) {
    return forEachValueOfHeader(code, func);
  } else {
    ITERATE_OVER_STRINGS(name, {
      if (func(headerValues_[pos])) {
        return true;
      }
    });
    return false;
  }
}

template <typename LAMBDA> // const string & -> bool
bool HTTPHeaders::forEachValueOfHeader(HTTPHeaderCode code, LAMBDA func) const {
  ITERATE_OVER_CODES(code, {
    if (func(headerValues_[pos])) {
      return true;
    }
  });
  return false;
}

template <typename T>
std::string HTTPHeaders::combine(const T& header,
                                 const std::string& separator) const {
  std::string combined = "";
  forEachValueOfHeader(header, [&] (const std::string& value) -> bool {
      if (combined.empty()) {
        combined.append(value);
      } else {
        combined.append(separator).append(value);
      }
      return false;
    });
  return combined;
}

// LAMBDA: (HTTPHeaderCode, const string&, const string&) -> bool
template <typename LAMBDA>
bool HTTPHeaders::removeByPredicate(LAMBDA func) {
  bool removed = false;
  for (size_t i = 0; i < codes_.size(); ++i) {
    if (codes_[i] == HTTP_HEADER_NONE ||
        !func(codes_[i], *headerNames_[i], headerValues_[i])) {
      continue;
    }

    if (codes_[i] == HTTP_HEADER_OTHER) {
      delete headerNames_[i];
      headerNames_[i] = nullptr;
    }

    codes_[i] = HTTP_HEADER_NONE;
    ++deletedCount_;
    removed = true;
  }

  return removed;
}

template <typename T> // either uint8_t or string
const std::string & HTTPHeaders::getSingleOrEmpty(const T& nameOrCode) const {
  const std::string* res = nullptr;
  forEachValueOfHeader(nameOrCode, [&] (const std::string& value) -> bool {
    if (res != nullptr) {
      res = nullptr;  // a second value is found
      return true;    // stop processing
    } else {
      res = &value;   // the first value is found
      return false;
    }
  });
  return res ? *res : empty_string;
}

template <typename T> // either uint8_t or string
std::string HTTPHeaders::getSingle(const T& nameOrCode,
                                   const std::string& dflt) const {
  const std::string* res = nullptr;
  forEachValueOfHeader(nameOrCode, [&] (const std::string& value) -> bool {
    if (res != nullptr) {
      res = nullptr;  // a second value is found
      return true;    // stop processing
    } else {
      res = &value;   // the first value is found
      return false;
    }
  });
  return res ? *res : dflt;
}

#ifndef RDD_HTTPHEADERS_IMPL
#undef ITERATE_OVER_CODES
#undef ITERATE_OVER_STRINGS
#endif // RDD_HTTPHEADERS_IMPL

} // namespace rdd
