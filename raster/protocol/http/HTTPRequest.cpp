/*
* Copyright (C) 2017, Yeolar
*/

#include "raster/net/NetUtil.h"
#include "raster/protocol/http/HTTPRequest.h"

namespace rdd {

HTTPRequest::HTTPRequest(
    StringPiece method_,
    StringPiece uri_,
    StringPiece version_,
    std::shared_ptr<HTTPHeaders> headers_,
    bool xheaders,
    const std::string& remoteIP_)
  : method(method_), uri(uri_), version(version_) {
  headers = headers_ ? headers_ : std::make_shared<HTTPHeaders>();
  host = headers->get("Host", "127.0.0.1");
  if (xheaders) {
    // Squid uses X-Forward-For, others use X-Real-Ip
    remoteIP = headers->get("X-Real-Ip",
                            headers->get("X-Forwarded-For", remoteIP_));
    if (!isValidIP(remoteIP))
      remoteIP = remoteIP_;
    // AWS uses X-Forwarded-Proto
    protocol = headers->get("X-Scheme", headers->get("X-Forwarded-Proto"));
    if (protocol != "http" && protocol != "https")
      protocol = "http";
  } else {
    remoteIP = remoteIP_;
    protocol = "http";
  }
  split('?', uri, path, query);
  parseQuery(arguments, query);
}

bool HTTPRequest::supportHTTP_1_1() const {
  return version == "HTTP/1.1";
}

bool HTTPRequest::keepAlive() const {
  auto conn = headers->get("Connection");
  if (supportHTTP_1_1())
    return !caseInsensitiveEqual(conn, "close");
  if (headers->has("Content-Length") || method == "HEAD" || method == "GET")
    return caseInsensitiveEqual(conn, "keep-alive");
  return false;
}

std::string HTTPRequest::fullURL() const {
  return to<std::string>(protocol, "://", host, uri);
}

Cookie* HTTPRequest::getCookies() {
  if (!cookies_) {
    cookies_ = std::make_shared<Cookie>();
    cookies_->load(headers->get("Cookie"));
  }
  return cookies_.get();
}

} // namespace rdd
