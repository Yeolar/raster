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

#pragma one

#include <atomic>
#include <map>
#include <memory>
#include <mutex>

#include "accelerator/Algorithm.h"
#include "accelerator/thread/SpinLock.h"

namespace rdd {

class Degrader {
 public:
  virtual bool needDemote() = 0;
};

class CountDegrader : public Degrader {
 public:
  CountDegrader() {}

  void setup(bool open, uint32_t limit, uint32_t gap);

  bool needDemote() override;

 private:
  std::atomic<bool> open_{false};
  uint32_t limit_{0};
  uint32_t gap_{0};
  uint32_t count_{0};
  time_t ts_{0};
  acc::SpinLock lock_;
};

class RateDegrader : public Degrader {
 public:
  RateDegrader() {}

  void setup(bool open, uint32_t limit, double rate);

  bool needDemote() override;

 private:
  std::atomic<bool> open_{false};
  double rate_{0.0};
  uint32_t limit_{0};
  uint32_t ticket_{0};
  time_t ts_{0};
  acc::SpinLock lock_;
};

class DegraderManager {
 public:
  DegraderManager() {}

  template <class Deg, class ...Args>
  void setupDegrader(const std::string& name, Args&&... args);

  bool hit(const std::string& name);

 private:
  std::map<std::string, std::unique_ptr<Degrader>> degraders_;
  std::mutex lock_;
};

template <class Deg, class ...Args>
void DegraderManager::setupDegrader(const std::string& name, Args&&... args) {
  std::lock_guard<std::mutex> guard(lock_);
  if (!acc::contain(degraders_, name)) {
    degraders_[name].reset(new Deg());
  }
  Degrader* d = degraders_[name].get();
  reinterpret_cast<Deg*>(d)->setup(std::forward<Args>(args)...);
}

} // namespace rdd

#define RDDDEG_HIT(name) \
  ::acc::Singleton< ::rdd::DegraderManager>::get()->hit(name)

