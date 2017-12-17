/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <map>
#include <string>

#include "raster/util/Range.h"

namespace rdd {

/**
 * Maintains Http-Header-Case for all keys.
 */
class HTTPHeaders {
public:
  HTTPHeaders() {}
  explicit HTTPHeaders(StringPiece sp);

  bool has(const std::string& name) const;

  void set(const std::string& name, const std::string& value);

  void add(const std::string& name, const std::string& value);

  std::string get(const std::string& name, const std::string& dflt = "") const;
  std::vector<std::string> getList(const std::string& name) const;

  void clearHeadersFor304();

  std::string normalizeName(const std::string& name) const;

  std::string str() const;

private:
  std::multimap<std::string, std::string> data_;
};

} // namespace rdd
