/*
 * Copyright 2017 Facebook, Inc.
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

#include <atomic>
#include <string>
#include <thread>

#include "accelerator/thread/ThreadUtil.h"
#include "accelerator/Conv.h"
#include "accelerator/Function.h"

namespace rdd {

class ThreadFactory {
 public:
  ThreadFactory(acc::StringPiece prefix)
    : prefix_(prefix.str()) {}

  std::thread newThread(acc::VoidFunc&& func) {
    auto name = acc::to<std::string>(prefix_, suffix_++);
    return std::thread(
        [&] () {
        acc::setCurrentThreadName(name);
          func();
        });
  }

  void setNamePrefix(acc::StringPiece prefix) {
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
