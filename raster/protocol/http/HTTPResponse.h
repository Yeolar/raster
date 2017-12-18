/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <memory>
#include <string>

#include "raster/io/IOBuf.h"
#include "raster/protocol/http/Cookie.h"
#include "raster/protocol/http/HTTPHeaders.h"

namespace rdd {

class HTTPResponse {
public:
  HTTPResponse() {}

  std::string computeEtag() const;

  void prependHeaders(StringPiece version);

  void appendData(StringPiece sp);

  std::unique_ptr<IOBuf> data;
  int statusCode;
  HTTPHeaders headers;
  std::shared_ptr<Cookie> cookies;
};

} // namespace rdd
