/*
 * Copyright (c) 2015, Facebook, Inc.
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

#define RDD_HTTPHEADERS_IMPL
#include "raster/protocol/http/HTTPHeaders.h"

#include <vector>

#include "accelerator/Logging.h"

namespace raster {

const std::string empty_string;
const std::string HTTPHeaders::COMBINE_SEPARATOR = ", ";

std::bitset<256>& HTTPHeaders::entityHeaderCodes() {
  static std::bitset<256> entityHeaderCodes;
  return entityHeaderCodes;
}

std::bitset<256>& HTTPHeaders::perHopHeaderCodes() {
  static std::bitset<256> perHopHeaderCodes;
  return perHopHeaderCodes;
}

void HTTPHeaders::initGlobals() {
  auto& entityHeaders = entityHeaderCodes();
  entityHeaders[HTTP_HEADER_ALLOW] = true;
  entityHeaders[HTTP_HEADER_CONTENT_ENCODING] = true;
  entityHeaders[HTTP_HEADER_CONTENT_LANGUAGE] = true;
  entityHeaders[HTTP_HEADER_CONTENT_LENGTH] = true;
  entityHeaders[HTTP_HEADER_CONTENT_MD5] = true;
  entityHeaders[HTTP_HEADER_CONTENT_RANGE] = true;
  entityHeaders[HTTP_HEADER_CONTENT_TYPE] = true;
  entityHeaders[HTTP_HEADER_LAST_MODIFIED] = true;

  auto& perHopHeaders = perHopHeaderCodes();
  perHopHeaders[HTTP_HEADER_CONNECTION] = true;
  perHopHeaders[HTTP_HEADER_KEEP_ALIVE] = true;
  perHopHeaders[HTTP_HEADER_PROXY_AUTHENTICATE] = true;
  perHopHeaders[HTTP_HEADER_PROXY_AUTHORIZATION] = true;
  perHopHeaders[HTTP_HEADER_PROXY_CONNECTION] = true;
  perHopHeaders[HTTP_HEADER_TE] = true;
  perHopHeaders[HTTP_HEADER_TRAILER] = true;
  perHopHeaders[HTTP_HEADER_TRANSFER_ENCODING] = true;
  perHopHeaders[HTTP_HEADER_UPGRADE] = true;
}

HTTPHeaders::HTTPHeaders()
  : deletedCount_(0) {
  codes_.reserve(kInitialVectorReserve);
  headerNames_.reserve(kInitialVectorReserve);
  headerValues_.reserve(kInitialVectorReserve);
}

void HTTPHeaders::parse(acc::StringPiece sp) {
  acc::StringPiece p = sp.split_step("\r\n");
  do {
    std::string line;
    while (!p.empty() && line.empty()) {
      toAppend(p, &line);
      p = sp.split_step("\r\n");
    }
    while (!p.empty() && isspace(p.front())) {
      toAppend(' ', ltrimWhitespace(p), &line);
      p = sp.split_step("\r\n");
    }
    acc::StringPiece name, value;
    if (split(':', line, name, value)) {
      add(name, trimWhitespace(value));
    }
  } while (!sp.empty());
}

void HTTPHeaders::add(acc::StringPiece name, acc::StringPiece value) {
  ACCCHECK(name.size());
  const HTTPHeaderCode code = HTTPCommonHeaders::hash(name.data(), name.size());
  codes_.push_back(code);
  headerNames_.push_back((code == HTTP_HEADER_OTHER)
      ? new std::string(name.data(), name.size())
      : HTTPCommonHeaders::getPointerToHeaderName(code));
  headerValues_.emplace_back(value.data(), value.size());
}

bool HTTPHeaders::exists(acc::StringPiece name) const {
  const HTTPHeaderCode code = HTTPCommonHeaders::hash(name.data(), name.size());
  if (code != HTTP_HEADER_OTHER) {
    return exists(code);
  } else {
    ITERATE_OVER_STRINGS(name, { return true; });
    return false;
  }
}

bool HTTPHeaders::exists(HTTPHeaderCode code) const {
  return memchr((void*)codes_.data(), code, codes_.size()) != nullptr;
}

size_t HTTPHeaders::getNumberOfValues(HTTPHeaderCode code) const {
  size_t count = 0;
  ITERATE_OVER_CODES(code, {
      (void)pos;
      ++count;
  });
  return count;
}

size_t HTTPHeaders::getNumberOfValues(acc::StringPiece name) const {
  size_t count = 0;
  forEachValueOfHeader(name, [&] (acc::StringPiece value) -> bool {
    ++count;
    return false;
  });
  return count;
}

bool HTTPHeaders::remove(acc::StringPiece name) {
  const HTTPHeaderCode code = HTTPCommonHeaders::hash(name.data(), name.size());
  if (code != HTTP_HEADER_OTHER) {
    return remove(code);
  } else {
    bool removed = false;
    ITERATE_OVER_STRINGS(name, {
      delete headerNames_[pos];
      codes_[pos] = HTTP_HEADER_NONE;
      removed = true;
      ++deletedCount_;
    });
    return removed;
  }
}

bool HTTPHeaders::remove(HTTPHeaderCode code) {
  bool removed = false;
  ITERATE_OVER_CODES(code, {
    codes_[pos] = HTTP_HEADER_NONE;
    removed = true;
    ++deletedCount_;
  });
  return removed;
}

void HTTPHeaders::disposeOfHeaderNames() {
  for (size_t i = 0; i < codes_.size(); ++i) {
    if (codes_[i] == HTTP_HEADER_OTHER) {
      delete headerNames_[i];
    }
  }
}

HTTPHeaders::~HTTPHeaders () {
  disposeOfHeaderNames();
}

HTTPHeaders::HTTPHeaders(const HTTPHeaders& hdrs)
  : codes_(hdrs.codes_),
    headerNames_(hdrs.headerNames_),
    headerValues_(hdrs.headerValues_),
    deletedCount_(hdrs.deletedCount_) {
  for (size_t i = 0; i < codes_.size(); ++i) {
    if (codes_[i] == HTTP_HEADER_OTHER) {
      headerNames_[i] = new std::string(*hdrs.headerNames_[i]);
    }
  }
}

HTTPHeaders::HTTPHeaders(HTTPHeaders&& hdrs) noexcept
  : codes_(std::move(hdrs.codes_)),
    headerNames_(std::move(hdrs.headerNames_)),
    headerValues_(std::move(hdrs.headerValues_)),
    deletedCount_(hdrs.deletedCount_) {
  hdrs.removeAll();
}

HTTPHeaders& HTTPHeaders::operator= (const HTTPHeaders& hdrs) {
  if (this != &hdrs) {
    disposeOfHeaderNames();
    codes_ = hdrs.codes_;
    headerNames_ = hdrs.headerNames_;
    headerValues_ = hdrs.headerValues_;
    deletedCount_ = hdrs.deletedCount_;
    for (size_t i = 0; i < codes_.size(); ++i) {
      if (codes_[i] == HTTP_HEADER_OTHER) {
        headerNames_[i] = new std::string(*hdrs.headerNames_[i]);
      }
    }
  }
  return *this;
}

HTTPHeaders& HTTPHeaders::operator= (HTTPHeaders&& hdrs) {
  if (this != &hdrs) {
    codes_ = std::move(hdrs.codes_);
    headerNames_ = std::move(hdrs.headerNames_);
    headerValues_ = std::move(hdrs.headerValues_);
    deletedCount_ = hdrs.deletedCount_;
    hdrs.removeAll();
  }
  return *this;
}

void HTTPHeaders::removeAll() {
  disposeOfHeaderNames();
  codes_.clear();
  headerNames_.clear();
  headerValues_.clear();
  deletedCount_ = 0;
}

size_t HTTPHeaders::size() const {
  return codes_.size() - deletedCount_;
}

void HTTPHeaders::copyTo(HTTPHeaders& hdrs) const {
  for (size_t i = 0; i < codes_.size(); ++i) {
    if (codes_[i] != HTTP_HEADER_NONE) {
      hdrs.codes_.push_back(codes_[i]);
      hdrs.headerNames_.push_back((codes_[i] == HTTP_HEADER_OTHER) ?
          new std::string(*headerNames_[i]) : headerNames_[i]);
      hdrs.headerValues_.push_back(headerValues_[i]);
    }
  }
}

bool HTTPHeaders::transferHeaderIfPresent(acc::StringPiece name,
                                          HTTPHeaders& strippedHeaders) {
  bool transferred = false;
  const HTTPHeaderCode code = HTTPCommonHeaders::hash(name.data(), name.size());
  if (code == HTTP_HEADER_OTHER) {
    ITERATE_OVER_STRINGS(name, {
      strippedHeaders.codes_.push_back(HTTP_HEADER_OTHER);
      // in the next line, ownership of pointer goes to strippedHeaders
      strippedHeaders.headerNames_.push_back(headerNames_[pos]);
      strippedHeaders.headerValues_.push_back(headerValues_[pos]);
      codes_[pos] = HTTP_HEADER_NONE;
      transferred = true;
      ++deletedCount_;
    });
  } else { // code != HTTP_HEADER_OTHER
    ITERATE_OVER_CODES(code, {
      strippedHeaders.codes_.push_back(code);
      strippedHeaders.headerNames_.push_back(headerNames_[pos]);
      strippedHeaders.headerValues_.push_back(headerValues_[pos]);
      codes_[pos] = HTTP_HEADER_NONE;
      transferred = true;
      ++deletedCount_;
    });
  }
  return transferred;
}

void HTTPHeaders::clearHeadersFor304() {
  // 304 responses should not contain entity headers (defined in
  // http://www.w3.org/Protocols/rfc2616/rfc2616-sec7.html#sec7.1)
  // not explicitly allowed by
  // http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html#sec10.3.5
  auto& entityHeaders = entityHeaderCodes();
  for (size_t i = 0; i < codes_.size(); ++i) {
    if (entityHeaders[codes_[i]]) {
      codes_[i] = HTTP_HEADER_NONE;
      ++deletedCount_;
      ACCLOG(V5) << "Remove entity header " << *headerNames_[i];
    }
  }
}

void HTTPHeaders::stripPerHopHeaders(HTTPHeaders& strippedHeaders) {
  forEachValueOfHeader(HTTP_HEADER_CONNECTION, [&]
                       (const std::string& value) -> bool {
    // Remove all headers specified in Connection header
    acc::StringPiece sp(value);
    while (!sp.empty()) {
      auto hdr = trimWhitespace(sp.split_step(','));
      if (!hdr.empty()) {
        if (transferHeaderIfPresent(hdr, strippedHeaders)) {
          ACCLOG(V3) << "Stripped connection-named hop-by-hop header " << hdr;
        }
      }
    }
    return false; // continue processing "connection" headers
  });

  // Strip hop-by-hop headers
  auto& perHopHeaders = perHopHeaderCodes();
  for (size_t i = 0; i < codes_.size(); ++i) {
    if (perHopHeaders[codes_[i]]) {
      strippedHeaders.codes_.push_back(codes_[i]);
      strippedHeaders.headerNames_.push_back(headerNames_[i]);
      strippedHeaders.headerValues_.push_back(headerValues_[i]);
      codes_[i] = HTTP_HEADER_NONE;
      ++deletedCount_;
      ACCLOG(V5) << "Stripped hop-by-hop header " << *headerNames_[i];
    }
  }
}

} // namespace raster
