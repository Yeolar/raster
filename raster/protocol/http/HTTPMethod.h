/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <string>

#include "raster/util/Range.h"

namespace rdd {

// Ordered by frequency to minimize time spent in iteration
#define HTTP_METHOD_GEN(x) \
  x(GET),                  \
  x(POST),                 \
  x(OPTIONS),              \
  x(DELETE),               \
  x(HEAD),                 \
  x(CONNECT),              \
  x(PUT),                  \
  x(TRACE)

#define HTTP_METHOD_ENUM(method) method

/**
 * See the definitions in RFC2616 5.1.1 for the source of this list.
 */
enum class HTTPMethod {
  HTTP_METHOD_GEN(HTTP_METHOD_ENUM)
};

#undef HTTP_METHOD_ENUM

HTTPMethod stringToMethod(StringPiece method);

const std::string& methodToString(HTTPMethod method);

std::ostream& operator<<(std::ostream& os, HTTPMethod method);

} // namespace rdd
