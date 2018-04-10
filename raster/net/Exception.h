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
#include <utility>

#include "accelerator/Conv.h"

namespace rdd {

class NetException : public std::exception {
 public:
  explicit NetException(std::string const& msg)
    : msg_(msg), code_(0) {}

  NetException(const NetException& other)
    : msg_(other.msg_), code_(other.code_) {}

  NetException(NetException&& other) noexcept
    : msg_(other.msg_), code_(other.code_) {}

  template<typename... Args>
  explicit NetException(Args&&... args)
    : msg_(acc::to<std::string>(std::forward<Args>(args)...)),
      code_(0) {}

  ~NetException(void) noexcept override {}

  const char* what(void) const noexcept override {
    return msg_.c_str();
  }

  void setCode(int code) { code_ = code; }
  int getCode() const { return code_; }

 private:
  const std::string msg_;
  int code_;
};

} // namespace rdd
