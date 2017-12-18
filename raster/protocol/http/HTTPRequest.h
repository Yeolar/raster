/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <memory>

#include "raster/protocol/http/Cookie.h"
#include "raster/protocol/http/HTTPMethod.h"
#include "raster/protocol/http/Util.h"
#include "raster/util/Range.h"

namespace rdd {

class HTTPRequest {
public:
  HTTPRequest(HTTPMethod method,
              StringPiece uri,
              StringPiece version,
              HTTPHeaders&& headers,
              bool xheaders = false,
              const std::string& remoteIP = "");

  bool supportHTTP_1_1() const;

  bool keepAlive() const;

  std::string fullURL() const;

  size_t contentLength() const;

  Cookie* getCookies();

  HTTPMethod method;
  StringPiece uri;
  StringPiece version;
  HTTPHeaders headers;
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

std::ostream& operator<<(std::ostream& os, const HTTPRequest& req);

} // namespace rdd
