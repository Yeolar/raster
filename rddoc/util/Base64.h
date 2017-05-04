/*
 * Copyright (C) 2017, Yeolar
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

}

#include "Base64-inl.h"
