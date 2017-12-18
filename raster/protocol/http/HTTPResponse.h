/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <memory>
#include <string>

#include "raster/io/Cursor.h"
#include "raster/io/IOBuf.h"
#include "raster/net/Protocol.h"
#include "raster/protocol/http/Cookie.h"
#include "raster/protocol/http/HTTPException.h"
#include "raster/protocol/http/HTTPHeaders.h"

namespace rdd {

class HTTPResponse {
public:
  HTTPResponse() {}

  std::string computeEtag() const;

  void prependHeaders(StringPiece version);

  template<class... Ts>
  void appendData(const Ts&... vs) {
    appendDataImpl(vs...);
  }

  std::unique_ptr<IOBuf> data;
  int statusCode;
  HTTPHeaders headers;
  std::shared_ptr<Cookie> cookies;

private:
  template<class T>
  void appendDataImpl(const T& v) {
    rdd::io::Appender appender(data.get(), Protocol::CHUNK_SIZE);
    appender(StringPiece(v));
  }

  template<class T, class... Ts>
  void appendDataImpl(const T& v, const Ts&... vs) {
    appendDataImpl(v);
    appendDataImpl(vs...);
  }
};

} // namespace rdd
