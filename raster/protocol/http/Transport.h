/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/protocol/http/HTTPMessage.h"

namespace rdd {

class HTTPTransport {
public:
  virtual void sendHeaders(const HTTPMessage& headers,
                           HTTPHeaderSize* size) = 0;

  virtual size_t sendBody(std::unique_ptr<IOBuf> body, bool includeEOM) = 0;

  virtual size_t sendChunkHeader(size_t length) = 0;

  virtual size_t sendChunkTerminator() = 0;

  virtual size_t sendTrailers(const HTTPHeaders& trailers) = 0;

  virtual size_t sendEOM() = 0;

  virtual size_t sendAbort() = 0;
};

} // namespace rdd
