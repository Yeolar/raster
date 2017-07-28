/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <atomic>
#include <string>
#include <thread>
#include "rddoc/util/Conv.h"
#include "rddoc/util/Function.h"
#include "rddoc/util/ThreadUtil.h"

namespace rdd {

class ThreadFactory {
public:
  ThreadFactory(const std::string& prefix)
    : prefix_(prefix) {}

  std::thread createThread(VoidFunc&& func) {
    auto thread = std::thread(std::move(func));
    setThreadName(thread.native_handle(), to<std::string>(prefix_, suffix_++));
    return thread;
  }

  void setNamePrefix(const std::string& prefix) { prefix_ = prefix; }
  std::string namePrefix() const { return prefix_; }

private:
  std::string prefix_;
  std::atomic<uint64_t> suffix_{0};
};

} // namespace rdd
