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

#include <condition_variable>
#include <mutex>

namespace rdd {

class Waiter {
 public:
  Waiter() : signal_(false) {}

  void wait() {
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait(lock, [&] { return signal_; });
    signal_ = false;
  }

  void wait(uint64_t timeout) {
    auto t = std::chrono::microseconds(timeout);
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait_for(lock, t, [&] { return signal_; });
    signal_ = false;
  }

  void notify() {
    std::unique_lock<std::mutex> lock(mtx_);
    signal_ = true;
    cv_.notify_all();
  }

  void notifyOne() {
    std::unique_lock<std::mutex> lock(mtx_);
    signal_ = true;
    cv_.notify_one();
  }

 private:
  bool signal_;
  std::condition_variable cv_;
  std::mutex mtx_;
};

} // namespace rdd
