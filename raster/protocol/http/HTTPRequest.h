/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <memory>

#include "raster/protocol/http/Cookie.h"
#include "raster/protocol/http/Util.h"
#include "raster/util/Range.h"

namespace rdd {

class HTTPRequest {
public:
  HTTPRequest(StringPiece method,
              StringPiece uri,
              StringPiece version = "HTTP/1.0",
              std::shared_ptr<HTTPHeaders> headers = nullptr,
              bool xheaders = false,
              const std::string& remoteIP = "");

  bool supportHTTP_1_1() const;

  bool keepAlive() const;

  std::string fullURL() const;

  Cookie* getCookies();

  StringPiece method;
  StringPiece uri;
  StringPiece version;
  std::shared_ptr<HTTPHeaders> headers;
  std::string remoteIP;
  std::string protocol;
  std::string host;
  StringPiece body;
  StringPiece path;
  StringPiece query;
  URLQuery arguments;
  std::multimap<std::string, HTTPFile> files;

private:
  std::shared_ptr<Cookie> cookies_;
};

} // namespace rdd
