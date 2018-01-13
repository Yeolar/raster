/*
 * Copyright 2017 Facebook, Inc.
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <functional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "raster/util/Conv.h"
#include "raster/util/Hash.h"
#include "raster/util/String.h"

namespace rdd {

/**
 * Class representing a URI.
 *
 * Consider http://www.facebook.com/foo/bar?key=foo#anchor
 *
 * The URI is broken down into its parts: scheme ("http"), authority
 * (ie. host and port, in most cases: "www.facebook.com"), path
 * ("/foo/bar"), query ("key=foo") and fragment ("anchor").  The scheme is
 * lower-cased.
 *
 * If this Uri represents a URL, note that, to prevent ambiguity, the component
 * parts are NOT percent-decoded; you should do this yourself with
 * uriUnescape() (for the authority and path) and uriUnescape(...,
 * UriEscapeMode::QUERY) (for the query, but probably only after splitting at
 * '&' to identify the individual parameters).
 */
class Uri {
 public:
  /**
   * Parse a Uri from a string.  Throws std::invalid_argument on parse error.
   */
  explicit Uri(StringPiece str);

  const std::string& scheme() const { return scheme_; }
  const std::string& username() const { return username_; }
  const std::string& password() const { return password_; }
  /**
   * Get host part of URI. If host is an IPv6 address, square brackets will be
   * returned, for example: "[::1]".
   */
  const std::string& host() const { return host_; }
  /**
   * Get host part of URI. If host is an IPv6 address, square brackets will not
   * be returned, for exmaple "::1"; otherwise it returns the same thing as
   * host().
   *
   * hostname() is what one needs to call if passing the host to any other tool
   * or API that connects to that host/port; e.g. getaddrinfo() only understands
   * IPv6 host without square brackets
   */
  std::string hostname() const;
  uint16_t port() const { return port_; }
  const std::string& path() const { return path_; }
  const std::string& query() const { return query_; }
  const std::string& fragment() const { return fragment_; }

  std::string authority() const;

  template <class String>
  String toString() const;

  std::string str() const { return toString<std::string>(); }

  void setPort(uint16_t port) {
    hasAuthority_ = true;
    port_ = port;
  }

  /**
   * Get query parameters as key-value pairs.
   * e.g. for URI containing query string:  key1=foo&key2=&key3&=bar&=bar=
   * In returned list, there are 3 entries:
   *     "key1" => "foo"
   *     "key2" => ""
   *     "key3" => ""
   * Parts "=bar" and "=bar=" are ignored, as they are not valid query
   * parameters. "=bar" is missing parameter name, while "=bar=" has more than
   * one equal signs, we don't know which one is the delimiter for key and
   * value.
   *
   * Note, this method is not thread safe, it might update internal state, but
   * only the first call to this method update the state. After the first call
   * is finished, subsequent calls to this method are thread safe.
   *
   * @return  query parameter key-value pairs in a vector, each element is a
   *          pair of which the first element is parameter name and the second
   *          one is parameter value
   */
  const std::vector<std::pair<std::string, std::string>>& getQueryParams();

 private:
  std::string scheme_;
  std::string username_;
  std::string password_;
  std::string host_;
  bool hasAuthority_;
  uint16_t port_;
  std::string path_;
  std::string query_;
  std::string fragment_;
  std::vector<std::pair<std::string, std::string>> queryParams_;
};

template <class String>
String Uri::toString() const {
  String str;
  if (hasAuthority_) {
    toAppend(scheme_, "://", &str);
    if (!password_.empty()) {
      toAppend(username_, ":", password_, "@", &str);
    } else if (!username_.empty()) {
      toAppend(username_, "@", &str);
    }
    toAppend(host_, &str);
    if (port_ != 0) {
      toAppend(":", port_, &str);
    }
  } else {
    toAppend(scheme_, ":", &str);
  }
  toAppend(path_, &str);
  if (!query_.empty()) {
    toAppend("?", query_, &str);
  }
  if (!fragment_.empty()) {
    toAppend("#", fragment_, &str);
  }
  return str;
}

namespace uri_detail {

typedef std::tuple<
    const std::string&,
    const std::string&,
    const std::string&,
    const std::string&,
    uint16_t,
    const std::string&,
    const std::string&,
    const std::string&> UriTuple;

inline UriTuple as_tuple(const rdd::Uri& k) {
  return UriTuple(
      k.scheme(),
      k.username(),
      k.password(),
      k.host(),
      k.port(),
      k.path(),
      k.query(),
      k.fragment());
}

} // namespace uri_detail

} // namespace rdd

namespace std {

template <>
struct hash<rdd::Uri> {
  std::size_t operator()(const rdd::Uri& k) const {
    return std::hash<rdd::uri_detail::UriTuple>()(
        rdd::uri_detail::as_tuple(k));
  }
};

template <>
struct equal_to<rdd::Uri> {
  bool operator()(const rdd::Uri& a, const rdd::Uri& b) const {
    return rdd::uri_detail::as_tuple(a) == rdd::uri_detail::as_tuple(b);
  }
};

} // namespace std
