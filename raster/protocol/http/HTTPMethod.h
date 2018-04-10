/*
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

#include <string>

#include "accelerator/Range.h"

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

HTTPMethod stringToMethod(acc::StringPiece method);

const std::string& methodToString(HTTPMethod method);

std::ostream& operator<<(std::ostream& os, HTTPMethod method);

} // namespace rdd
