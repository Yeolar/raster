/*
 * Copyright (c) 2015, Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <string>

#include "raster/util/Conv.h"
#include "raster/util/Logging.h"
#include "raster/util/Range.h"

namespace rdd {

/**
 * ParseURL can handle non-fully-formed URLs.
 * This class must not persist beyond the lifetime of the buffer
 * underlying the input StringPiece
 */
class ParseURL {
public:
  ParseURL() {}
  explicit ParseURL(StringPiece urlVal) {
    init(urlVal);
  }

  void init(StringPiece urlVal) {
    RDDCHECK(!initialized_);
    url_ = urlVal;
    parse();
    initialized_ = true;
  }

  StringPiece url() const { return url_; }
  StringPiece scheme() const { return scheme_; }
  std::string authority() const { return authority_; }
  StringPiece host() const { return host_; }
  uint16_t port() const { return port_; }
  StringPiece path() const { return path_; }
  StringPiece query() const { return query_; }
  StringPiece fragment() const { return fragment_; }

  bool valid() const { return valid_; }
  bool hasHost() const { return valid() && !host_.empty(); }

  std::string hostAndPort() const {
    std::string rc = host_.str();
    if (port_ != 0) {
      toAppend(":", port_, &rc);
    }
    return rc;
  }

  StringPiece hostNoBrackets() {
    stripBrackets();
    return hostNoBrackets_;
  }

  bool hostIsIPAddress();

  void stripBrackets();

private:
  void parse();

  void parseNonFully();

  bool parseAuthority();

  StringPiece url_;
  StringPiece scheme_;
  std::string authority_;
  StringPiece host_;
  StringPiece hostNoBrackets_;
  StringPiece path_;
  StringPiece query_;
  StringPiece fragment_;
  uint16_t port_{0};
  bool valid_{false};
  bool initialized_{false};
};

} // namespace rdd
