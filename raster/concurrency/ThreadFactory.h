/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <atomic>
#include <string>
#include <thread>

#include "raster/thread/ThreadUtil.h"
#include "raster/util/Conv.h"
#include "raster/util/Function.h"

namespace rdd {

class ThreadFactory {
 public:
  ThreadFactory(StringPiece prefix)
    : prefix_(prefix.str()) {}

  std::thread newThread(VoidFunc&& func) {
    auto name = to<std::string>(prefix_, suffix_++);
    return std::thread(
        [&] () {
          setCurrentThreadName(name);
          func();
        });
  }

  void setNamePrefix(StringPiece prefix) {
    prefix_ = prefix.str();
  }

  std::string namePrefix() const {
    return prefix_;
  }

 private:
  std::string prefix_;
  std::atomic<uint64_t> suffix_{0};
};

} // namespace rdd
