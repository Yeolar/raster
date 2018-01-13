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
  void wait() const {
    std::unique_lock<std::mutex> lock(mtx_);
    cond_.wait(lock, [&] { return signal_; });
    signal_ = false;
  }

  template <class Rep, class Period>
  void wait(const std::chrono::duration<Rep, Period>& duration) const {
    std::unique_lock<std::mutex> lock(mtx_);
    cond_.wait_for(lock, duration, [&] { return signal_; });
    signal_ = false;
  }

  void notify_one() const {
    {
      std::unique_lock<std::mutex> lock(mtx_);
      signal_ = true;
    }
    cond_.notify_one();
  }

  void notify_all() const {
    {
      std::unique_lock<std::mutex> lock(mtx_);
      signal_ = true;
    }
    cond_.notify_all();
  }

 private:
  mutable bool signal_{false};
  mutable std::condition_variable cond_;
  mutable std::mutex mtx_;
};

} // namespace rdd
