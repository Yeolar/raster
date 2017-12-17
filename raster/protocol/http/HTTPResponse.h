/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <string>

namespace rdd {

class HTTPResponse {
public:
  HTTPResponse() {}

  std::string data;
  int code;
};

} // namespace rdd
