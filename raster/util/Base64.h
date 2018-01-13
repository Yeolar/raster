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

namespace rdd {

enum class Base64EncodeMode : unsigned char {
  COMMON = 0,
  URI = 1
};

template <class InputString>
bool base64Encode(const InputString& input,
                  std::string& output,
                  Base64EncodeMode mode = Base64EncodeMode::COMMON);

template <class InputString>
bool base64Decode(const InputString& input,
                  std::string& output,
                  Base64EncodeMode mode = Base64EncodeMode::COMMON);

} // namespace rdd

#include "Base64-inl.h"
