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
    HTTPHeaders&& headers_,
    bool xheaders,
    const std::string& remoteIP_)
  : method(method_),
    uri(uri_),
    version(version_),
    headers(std::move(headers_)) {
  host = headers.getSingle(HTTP_HEADER_HOST, "127.0.0.1");
  if (xheaders) {
    // Squid uses X-Forward-For, others use X-Real-Ip
    remoteIP = headers.getSingle(HTTP_HEADER_X_REAL_IP,
               headers.getSingle(HTTP_HEADER_X_FORWARDED_FOR, remoteIP_));
    if (!isValidIP(remoteIP))
      remoteIP = remoteIP_;
    // AWS uses X-Forwarded-Proto
    protocol = headers.getSingle(HTTP_HEADER_X_SCHEME,
               headers.getSingleOrEmpty(HTTP_HEADER_X_FORWARDED_PROTO));
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
  auto connectionHdr = headers.combine(HTTP_HEADER_CONNECTION);
  if (supportHTTP_1_1())
    return !caseInsensitiveEqual(connectionHdr, "close");
  if (headers.exists(HTTP_HEADER_CONTENT_LENGTH)
      || method == "HEAD" || method == "GET")
    return caseInsensitiveEqual(connectionHdr, "keep-alive");
  return false;
}

std::string HTTPRequest::fullURL() const {
  return to<std::string>(protocol, "://", host, uri);
}

Cookie* HTTPRequest::getCookies() {
  if (!cookies_) {
    cookies_ = std::make_shared<Cookie>();
    cookies_->load(headers.combine(HTTP_HEADER_COOKIE));
  }
  return cookies_.get();
}

} // namespace rdd
