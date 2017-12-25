/*
 * Copyright (c) 2015, Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <array>
#include <map>
#include <mutex>
#include <string>
#include <boost/variant.hpp>

#include "raster/net/NetUtil.h"
#include "raster/protocol/http/HTTPHeaders.h"
#include "raster/protocol/http/HTTPMethod.h"
#include "raster/protocol/http/ParseURL.h"
#include "raster/util/Logging.h"

namespace rdd {

/**
 * An HTTP request or response minus the body.
 *
 * Some of the methods on this class will assert if called from the wrong
 * context since they only make sense for a request or response. Make sure
 * you know what type of HTTPMessage this is before calling such methods.
 *
 * All header names stored in this class are case-insensitive.
 */
class HTTPMessage {
public:
  HTTPMessage();
  ~HTTPMessage();
  HTTPMessage(const HTTPMessage& message);
  HTTPMessage& operator=(const HTTPMessage& message);

  void setIsChunked(bool chunked) { chunked_ = chunked; }
  bool getIsChunked() const { return chunked_; }

  void setClientAddress(const Peer& addr) { request().clientAddr_ = addr; }
  const Peer& getClientAddress() const { return request().clientAddr_; }
  int getClientPort() const { return request().clientAddr_.port(); }

  void setDstAddress(const Peer& addr) { dstAddr_ = addr; }
  const Peer& getDstAddress() const { return dstAddr_; }
  int getDstPort() const { return dstAddr_.port(); }

  template <typename T> // T = string
  void setLocalIp(T&& ip) { localIP_ = std::forward<T>(ip); }
  const std::string& getLocalIp() const { return localIP_; }

  void setMethod(HTTPMethod method);
  void setMethod(StringPiece method);
  HTTPMethod getMethod() const;
  const std::string& getMethodString() const;

  /**
   * The <url> component from the initial "METHOD <url> HTTP/..." line. When
   * valid, this is a full URL, not just a path.
   */
  template <typename T> // T = string
  ParseURL setURL(T&& url) {
    RDDLOG(V5) << "setURL: " << url;

    // Set the URL, path, and query string parameters
    ParseURL u(url);
    if (u.valid()) {
      RDDLOG(V5) << "set path: " << u.path() << " query:" << u.query();
      request().path_ = u.path().str();
      request().query_ = u.query().str();
      unparseQueryParams();
    } else {
      RDDLOG(V4) << "Error in parsing URL: " << url;
    }

    request().url_ = std::forward<T>(url);
    return u;
  }
  // The template function above doesn't work with char*,
  // so explicitly convert to a string first.
  void setURL(const char* url) {
    setURL(std::string(url));
  }
  const std::string& getURL() const {
    return request().url_;
  }

  const std::string& getPath() const {
    return request().path_;
  }

  const std::string& getQueryString() const {
    return request().query_;
  }

  static const std::pair<uint8_t, uint8_t> kHTTPVersion10;
  static const std::pair<uint8_t, uint8_t> kHTTPVersion11;

  void setHTTPVersion(uint8_t major, uint8_t minor);
  const std::pair<uint8_t, uint8_t>& getHTTPVersion() const;

  template <typename T> // T = string
  void setStatusMessage(T&& msg) {
    response().statusMsg_ = std::forward<T>(msg);
  }
  const std::string& getStatusMessage() const {
    return response().statusMsg_;
  }

  const std::string& getVersionString() const {
    return versionStr_;
  }
  void setVersionString(const std::string& ver) {
    if (ver.size() != 3 ||
        ver[1] != '.' ||
        !isdigit(ver[0]) ||
        !isdigit(ver[2])) {
      return;
    }
    setHTTPVersion(ver[0] - '0', ver[2] - '0');
  }

  HTTPHeaders& getHeaders() { return headers_; }
  const HTTPHeaders& getHeaders() const { return headers_; }

  HTTPHeaders* getTrailers() { return trailers_.get(); }
  const HTTPHeaders* getTrailers() const { return trailers_.get(); }

  void setTrailers(std::unique_ptr<HTTPHeaders>&& trailers) {
    trailers_ = std::move(trailers);
  }

  /**
   * Decrements Max-Forwards header, when present on OPTIONS or TRACE methods.
   */
  int processMaxForwards();

  bool isHTTP1_0() const;
  bool isHTTP1_1() const;

  bool is1xxResponse() const { return (getStatusCode() / 100) == 1; }

  static std::string formatDateHeader();

  void ensureHostHeader();

  void setWantsKeepalive(bool wantsKeepaliveVal) {
    wantsKeepalive_ = wantsKeepaliveVal;
  }
  bool wantsKeepalive() const {
    return wantsKeepalive_;
  }

  bool trailersAllowed() const { return trailersAllowed_; }
  void setTrailersAllowed(bool trailersAllowedVal) {
    trailersAllowed_ = trailersAllowedVal;
  }

  bool hasTrailers() const {
    return trailersAllowed_ && trailers_ && trailers_->size() > 0;
  }

  void setStatusCode(uint16_t status);
  uint16_t getStatusCode() const;

  /**
   * Fill in the fields for a response message header that the server will
   * send directly to the client.
   */
  void constructDirectResponse(const std::pair<uint8_t, uint8_t>& version,
                               const int statusCode,
                               const std::string& statusMsg,
                               int contentLength = 0);
  void constructDirectResponse(const std::pair<uint8_t, uint8_t>& version,
                               int contentLength = 0);

  bool hasQueryParam(const std::string& name) const;

  const std::string* getQueryParamPtr(const std::string& name) const;

  const std::string& getQueryParam(const std::string& name) const;

  int getIntQueryParam(const std::string& name) const;
  int getIntQueryParam(const std::string& name, int defval) const;

  std::string getDecodedQueryParam(const std::string& name) const;

  const std::map<std::string, std::string>& getQueryParams() const;

  bool setQueryString(const std::string& query);

  bool removeQueryParam(const std::string& name);

  bool setQueryParam(const std::string& name, const std::string& value);

  /**
   * Get the cookie with the specified name.
   *
   * Returns a StringPiece to the cookie value, or an empty StringPiece if
   * there is no cookie with the specified name.  The returned cookie is
   * only valid as long as the Cookie Header in HTTPMessage object exists.
   * Applications should make sure they call unparseCookies() when editing
   * the Cookie Header, so that the StringPiece references are cleared.
   */
  const StringPiece getCookie(const std::string& name) const;

  void dumpMessage(int verbosity) const;
  void atomicDumpMessage(int verbosity) const;

  /**
   * Interact with headers that are defined to be per-hop.
   *
   * It is expected that during request processing, stripPerHopHeaders() will
   * be called before the message is proxied to the other connection.
   */
  void stripPerHopHeaders();

  const HTTPHeaders& getStrippedPerHopHeaders() const {
    return strippedPerHopHeaders_;
  }

  void setSecure(bool secure) { secure_ = secure; }
  bool isSecure() const { return secure_; }
  int getSecureVersion() const { return sslVersion_; }
  const char* getSecureCipher() const { return sslCipher_; }
  void setSecureInfo(int ver, const char* cipher) {
    // cipher is a static const char* provided and managed by openssl lib
    sslVersion_ = ver; sslCipher_ = cipher;
  }

  void setSeqNo(int32_t seqNo) { seqNo_ = seqNo; }
  int32_t getSeqNo() const { return seqNo_; }

  void setIngressHeaderSize(const HTTPHeaderSize& size) { size_ = size; }
  const HTTPHeaderSize& getIngressHeaderSize() const { return size_; }

  uint64_t getStartTime() const { return startTime_; }

  /**
   * Check if a particular token value is present in a header that consists of
   * a list of comma separated tokens.  (e.g., a header with a #rule
   * body as specified in the RFC 2616 BNF notation.)
   */
  bool checkForHeaderToken(const HTTPHeaderCode headerCode,
                           char const* token,
                           bool caseSensitive) const;

  /*
   * Ideally HTTPMessage should automatically forget about the current parsed
   * cookie state whenever a Cookie header is changed.  However, at the moment
   * callers have to explicitly call unparseCookies() after modifying the
   * cookie headers.
   */
  void unparseCookies();

  static const char* getDefaultReason(uint16_t status);

  bool computeKeepalive() const;

  bool isRequest() const {
    return fields_.which() == 1;
  }

  bool isResponse() const {
    return fields_.which() == 2;
  }

  static void splitNameValuePieces(
      const std::string& input,
      char pairDelim,
      char valueDelim,
      std::function<void(StringPiece, StringPiece)> callback);

  static void splitNameValue(
      const std::string& input,
      char pairDelim,
      char valueDelim,
      std::function<void(std::string&&, std::string&&)> callback);

  /**
   * Form the URL from the individual components.
   * url -> {scheme}://{authority}{path}?{query}#{fragment}
   */
  static std::string createUrl(const StringPiece scheme,
                               const StringPiece authority,
                               const StringPiece path,
                               const StringPiece query,
                               const StringPiece fragment);

  static std::string createQueryString(
      const std::map<std::string, std::string>& params, uint32_t maxSize);

private:
  void parseCookies() const;

  void parseQueryParams() const;
  void unparseQueryParams();

  struct Request {
    Peer clientAddr_;
    HTTPMethod method_;
    std::string path_;
    std::string query_;
    std::string url_;
  };

  struct Response {
    uint16_t status_;
    std::string statusStr_;
    std::string statusMsg_;
  };

  uint64_t startTime_;
  int32_t seqNo_;

  Peer dstAddr_;

  std::string localIP_;
  std::string versionStr_;

  mutable boost::variant<boost::blank, Request, Response> fields_;

  Request& request() {
    DCHECK(fields_.which() == 0 || fields_.which() == 1) << fields_.which();
    if (fields_.which() == 0) fields_ = Request();
    return boost::get<Request>(fields_);
  }

  const Request& request() const {
    DCHECK(fields_.which() == 0 || fields_.which() == 1) << fields_.which();
    if (fields_.which() == 0) fields_ = Request();
    return boost::get<const Request>(fields_);
  }

  Response& response() {
    DCHECK(fields_.which() == 0 || fields_.which() == 2) << fields_.which();
    if (fields_.which() == 0) fields_ = Response();
    return boost::get<Response>(fields_);
  }

  const Response& response() const {
    DCHECK(fields_.which() == 0 || fields_.which() == 2) << fields_.which();
    if (fields_.which() == 0) fields_ = Response();
    return boost::get<const Response>(fields_);
  }

  /*
   * Cookies and query parameters
   * These are mutable since we parse them lazily in getCookie() and
   * getQueryParam()
   */
  mutable std::map<StringPiece, StringPiece> cookies_;
  // TODO: use StringPiece for queryParams_ and delete splitNameValue()
  mutable std::map<std::string, std::string> queryParams_;

  std::pair<uint8_t, uint8_t> version_;
  HTTPHeaders headers_;
  HTTPHeaders strippedPerHopHeaders_;
  HTTPHeaderSize size_;
  std::unique_ptr<HTTPHeaders> trailers_;

  int sslVersion_;
  const char* sslCipher_;

  mutable bool parsedCookies_:1;
  mutable bool parsedQueryParams_:1;
  bool chunked_:1;
  bool wantsKeepalive_:1;
  bool trailersAllowed_:1;

  bool secure_:1;   // Whether the message is received in HTTPS.

  static std::mutex mutexDump_;
};

template<typename Str>
std::string stripCntrlChars(const Str& str) {
  std::string res;
  res.reserve(str.length());
  for (size_t i = 0; i < str.size(); ++i) {
    if (!(str[i] <= 0x1F || str[i] == 0x7F)) {
      res += str[i];
    }
  }
  return res;
}

} // namespace rdd
