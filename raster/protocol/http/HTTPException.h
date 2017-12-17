/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <stdexcept>

#include "raster/util/Conv.h"

namespace rdd {

class HTTPException : public std::exception {
public:
  explicit HTTPException(int code, const std::string& msg)
    : msg_(msg), code_(code) {}

  template<typename... Args>
  explicit HTTPException(int code, Args&&... args)
    : msg_(to<std::string>(std::forward<Args>(args)...)), code_(code) {}

  HTTPException(const HTTPException& other)
    : msg_(other.msg_), code_(other.code_) {}

  HTTPException(HTTPException&& other) noexcept
    : msg_(other.msg_), code_(other.code_) {}

  virtual ~HTTPException() noexcept {}

  virtual const char* what() const noexcept {
    return msg_.c_str();
  }

  void setCode(int code) { code_ = code; }
  int getCode() const { return code_; }

  bool emptyMessage() const { return msg_.empty(); }

private:
  const std::string msg_;
  int code_;
};

std::ostream& operator<<(std::ostream& os, const HTTPException& ex);

} // namespace rdd
