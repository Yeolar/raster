/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <map>
#include <memory>
#include <string>

#include "raster/util/Range.h"

namespace rdd {

/**
 * Abstract a key/value pair, which has some RFC 2109 attributes.
 *
 * The valid RFC 2109 attributes, which are:
 *
 *  - expires
 *  - path
 *  - comment
 *  - domain
 *  - max-age
 *  - secure
 *  - version
 *  - httponly
 *
 * The attribute httponly specifies that the cookie is only transfered
 * in HTTP requests, and is not accessible through JavaScript.
 * This is intended to mitigate some forms of cross-site scripting.
 *
 * The keys are case-insensitive.
 */
struct CookieMorsel {
  std::string key;
  std::string value;
  std::string codedValue;
  std::map<std::string, std::string> attributes;

  static bool isReserved(StringPiece key);

  void set(const std::string& key,
           const std::string& value,
           const std::string& codedValue);

  void setAttr(const std::string& key, const std::string& value);

  std::string str();
};

struct Cookie {
  std::map<std::string, std::shared_ptr<CookieMorsel>> data;

  void load(StringPiece sp);

  void clear() { data.clear(); }

  void set(const std::string& key, const std::string& value);

  std::string str();

  static std::string quote(StringPiece value);
  static std::string unquote(StringPiece value);

private:
  void set(const std::string& key,
           const std::string& value,
           const std::string& codedValue);
};

}  // namespace rdd
