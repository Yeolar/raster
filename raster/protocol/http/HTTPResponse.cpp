/*
 * Copyright (C) 2017, Yeolar
 */

#include <sstream>

#include "raster/protocol/http/HTTPResponse.h"
#include "raster/protocol/http/Util.h"
#include "raster/ssl/OpenSSLHash.h"
#include "raster/util/Conv.h"

namespace rdd {

std::string HTTPResponse::computeEtag() const {
  std::string out;
  std::vector<uint8_t> hash(32);
  OpenSSLHash::sha1(range(hash), *data);
  hexlify(hash, out);
  return '"' + out + '"';
}

void HTTPResponse::prependHeaders(StringPiece version) {
  std::string header;
  toAppend(version, ' ', statusCode, ' ',
           getResponseW3CName(statusCode), "\r\n", &header);
  headers.forEach([&](const std::string& k, const std::string& v) {
    toAppend(k, ": ", v, "\r\n", &header);
  });
  toAppend(cookies.str(), "\r\n\r\n", &header);

  auto buf = IOBuf::create(header.size());
  rdd::io::Appender appender(buf.get(), 0);
  appender(StringPiece(header));
  data.swap(buf);
  data->appendChain(std::move(buf));
}

} // namespace rdd
