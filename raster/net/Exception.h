/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <string>
#include <utility>

#include "raster/util/Conv.h"

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
    : msg_(to<std::string>(std::forward<Args>(args)...)),
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
