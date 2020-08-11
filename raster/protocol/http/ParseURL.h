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

#pragma once

#include <string>

#include "accelerator/Conv.h"
#include "accelerator/Logging.h"
#include "accelerator/Range.h"

namespace raster {

/**
 * ParseURL can handle non-fully-formed URLs.
 * This class must not persist beyond the lifetime of the buffer
 * underlying the input acc::StringPiece
 */
class ParseURL {
 public:
  ParseURL() {}
  explicit ParseURL(acc::StringPiece urlVal) {
    init(urlVal);
  }

  void init(acc::StringPiece urlVal) {
    ACCCHECK(!initialized_);
    url_ = urlVal;
    parse();
    initialized_ = true;
  }

  acc::StringPiece url() const { return url_; }
  acc::StringPiece scheme() const { return scheme_; }
  acc::StringPiece authority() const { return authority_; }
  acc::StringPiece host() const { return host_; }
  uint16_t port() const { return port_; }
  acc::StringPiece path() const { return path_; }
  acc::StringPiece query() const { return query_; }
  acc::StringPiece fragment() const { return fragment_; }

  bool valid() const { return valid_; }
  bool hasHost() const { return valid() && !host_.empty(); }

  std::string hostAndPort() const {
    std::string rc = host_.str();
    if (port_ != 0) {
      acc::toAppend(":", port_, &rc);
    }
    return rc;
  }

  acc::StringPiece hostNoBrackets() {
    stripBrackets();
    return hostNoBrackets_;
  }

  bool hostIsIPAddress();

  void stripBrackets();

 private:
  void parse();

  void parseNonFully();

  bool parseAuthority();

  acc::StringPiece url_;
  acc::StringPiece scheme_;
  std::string authority_;
  acc::StringPiece host_;
  acc::StringPiece hostNoBrackets_;
  acc::StringPiece path_;
  acc::StringPiece query_;
  acc::StringPiece fragment_;
  uint16_t port_{0};
  bool valid_{false};
  bool initialized_{false};
};

} // namespace raster
